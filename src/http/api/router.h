#ifndef HTTP_ROUTER_H
#define HTTP_ROUTER_H

#include "api/error_codes.h"
#include "request.h"
#include "response.h"

typedef struct HttpRouter HttpRouter;

/*
 * Notes endpoint callback types.
 *
 * These callbacks will later be implemented in api/notes_handler.c.
 */
typedef HttpResponse (*HttpRouteHandler)(const HttpRequest *request, void *user_data);
typedef HttpResponse (*NotesCreateHandler)(const HttpRequest *request, void *user_data);
typedef HttpResponse (*NotesListHandler)(const HttpRequest *request, const char *tag, void *user_data);
typedef HttpResponse (*NotesGetHandler)(const HttpRequest *request, const char *note_id, void *user_data);
typedef HttpResponse (*NotesUpdateHandler)(const HttpRequest *request, const char *note_id, void *user_data);
typedef HttpResponse (*NotesDeleteHandler)(const HttpRequest *request, const char *note_id, void *user_data);

typedef struct HttpRouterConfig {
    NotesCreateHandler create_note;
    NotesListHandler list_notes;
    NotesGetHandler get_note;
    NotesUpdateHandler update_note;
    NotesDeleteHandler delete_note;
    void *notes_user_data;
} HttpRouterConfig;

/*
 * Create/destroy router instance.
 */
HttpRouter *httpRouterCreate(const HttpRouterConfig *config);
RetCode httpRouterDestroy(HttpRouter *router);

/*
 * Main router entrypoint.
 *
 * This matches the HttpRouteHandler signature from http/server.h, so it can be
 * passed directly into the server as:
 *
 *   .handler = httpRouterHandle,
 *   .handler_user_data = router
 */
HttpResponse httpRouterHandle(const HttpRequest *request, void *user_data);

#endif /* HTTP_ROUTER_H */