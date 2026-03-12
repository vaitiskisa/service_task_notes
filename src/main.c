
#include "api/notes_handler.h"
#include "api/router.h"
#include "api/server.h"
#include "api/notes_repository.h"
#include "api/notes_service.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_PORT     8080
#define DEFAULT_DATA_DIR "./data/notes"

static volatile sig_atomic_t g_should_stop = 0;

static void handleSignal(int sig)
{
    (void)sig;
    g_should_stop = 1;
}

static unsigned short parsePort(const char *arg)
{
    char *end = NULL;
    long port = strtol(arg, &end, 10);

    if(end == arg || *end != '\0' || port <= 0 || port > 65535) {
        return DEFAULT_PORT;
    }

    return (unsigned short)port;
}

int main(int argc, char *argv[])
{
    RetCode ret_code = RETCODE_OK;

    const char *data_dir = DEFAULT_DATA_DIR;
    unsigned short port = DEFAULT_PORT;

    if(argc >= 2) {
        port = parsePort(argv[1]);
    }

    if(argc >= 3) {
        data_dir = argv[2];
    }

    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);

    NotesRepository *repository = NULL;
    NotesService *service = NULL;
    NotesHandlerContext *notes_handler_ctx = NULL;
    HttpRouter *router = NULL;
    HttpServer *server = NULL;

    /*
     * Repository
     */
    repository = notesRepositoryCreate(data_dir);
    if(!repository) {
        fprintf(stderr, "Failed to create notes repository (dir=%s)\n", data_dir);
        ret_code = RETCODE_COMMON_ERROR;
        goto EXIT;
    }

    /*
     * Service
     */
    service = notesServiceCreate(repository);
    if(!service) {
        fprintf(stderr, "Failed to initialize notes service\n");
        ret_code = RETCODE_COMMON_ERROR;
        goto EXIT;
    }

    /*
     * Handler context
     */
    notes_handler_ctx = createNotesHandlerContext(service);
    if(!notes_handler_ctx) {
        fprintf(stderr, "Failed to create notes handler context\n");
        ret_code = RETCODE_COMMON_ERROR;
        goto EXIT;
    }

    /*
     * HTTP router
     */
    HttpRouterConfig router_config = { .create_note = createNoteHandler,
                                       .list_notes = listNotesHandler,
                                       .get_note = getNoteHandler,
                                       .update_note = updateNoteHandler,
                                       .delete_note = deleteNoteHandler,
                                       .notes_user_data = notes_handler_ctx };
    router = httpRouterCreate(&router_config);
    if(!router) {
        fprintf(stderr, "Failed to initialize HTTP router\n");
        ret_code = RETCODE_COMMON_ERROR;
        goto EXIT;
    }

    /*
     * HTTP server
     */
    HttpServerConfig server_config = {
        .port = port,
        .handler = httpRouterHandle,
        .handler_user_data = router,
    };
    server = httpServerCreate(&server_config);
    if(!server) {
        fprintf(stderr, "Failed to create HTTP server\n");
        ret_code = RETCODE_COMMON_ERROR;
        goto EXIT;
    }

    if(httpServerStart(server) != RETCODE_OK) {
        fprintf(stderr, "Failed to start HTTP server on port %u\n", port);
        ret_code = RETCODE_COMMON_ERROR;
        goto EXIT;
    }

    printf("Task Notes server started.\n");
    printf("Listening on: http://127.0.0.1:%u\n", port);
    printf("Data directory: %s\n", data_dir);
    printf("Press Ctrl+C to stop.\n");

    while(!g_should_stop) {
        /*
         * Keep the process alive. The actual work is done in the HTTP server thread, so we can just sleep here.
         */
        usleep(200000);
    }

    printf("\nStopping server...\n");

EXIT:
    if(server) {
        httpServerDestroy(server);
    }
    if(router) {
        httpRouterDestroy(router);
    }
    if(notes_handler_ctx) {
        destroyNotesHandlerContext(notes_handler_ctx);
    }
    if(service) {
        notesServiceDestroy(service);
    }
    if(repository) {
        notesRepositoryDestroy(repository);
    }
    return ret_code;
}
