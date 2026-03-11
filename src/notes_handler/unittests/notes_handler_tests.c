/**
 * @file notes_handler_tests.c
 * @brief Unit tests for notes handler endpoints.
 */
#include "api/notes_handler.h"
#include "api/json_utils.h"
#include "mock_functions.h"

#include <jansson.h>
#include <string.h>
#include <setjmp.h>
#include <cmocka.h>

/**
 * @brief Verifies successful POST /notes creates a note and returns 201.
 */
static void testCreateNoteSuccess(void **state)
{
    (void)state;
    resetMocks();

    fillNote(&g_create_out, "id-1", "Title", "Content", "work");

    NotesHandlerContext ctx = { .service = (NotesService *)0x1 };
    const char *body = "{\"title\":\"Title\",\"content\":\"Content\",\"tags\":[\"work\"]}";
    HttpRequest req = {
        .method = "POST",
        .url = "/notes",
        .path = "/notes",
        .query = NULL,
        .body = body,
        .body_size = strlen(body),
    };

    HttpResponse resp = createNoteHandler(&req, &ctx);
    assert_int_equal(resp.status_code, HTTP_STATUS_CREATED);
    assert_true(g_last_input_set);
    assert_string_equal(g_last_input.title, "Title");
    assert_string_equal(g_last_input.content, "Content");
    assert_true(noteHasTag(&g_last_input, "work"));

    httpResponseFree(&resp);
    noteFree(&g_create_out);
}

/**
 * @brief Verifies invalid POST /notes body returns 400.
 */
static void testCreateNoteInvalidBody(void **state)
{
    (void)state;
    resetMocks();

    NotesHandlerContext ctx = { .service = (NotesService *)0x1 };
    HttpRequest req = {
        .method = "POST",
        .url = "/notes",
        .path = "/notes",
        .query = NULL,
        .body = NULL,
        .body_size = 0,
    };

    HttpResponse resp = createNoteHandler(&req, &ctx);
    assert_int_equal(resp.status_code, HTTP_STATUS_BAD_REQUEST);
    httpResponseFree(&resp);
}

/**
 * @brief Verifies service errors during create map to 500.
 */
static void testCreateNoteServiceError(void **state)
{
    (void)state;
    resetMocks();

    g_ret_create = RETCODE_NOTES_SERVICE_NO_MEMORY;

    NotesHandlerContext ctx = { .service = (NotesService *)0x1 };
    const char *body = "{\"title\":\"Title\",\"content\":\"Content\"}";
    HttpRequest req = {
        .method = "POST",
        .url = "/notes",
        .path = "/notes",
        .query = NULL,
        .body = body,
        .body_size = strlen(body),
    };

    HttpResponse resp = createNoteHandler(&req, &ctx);
    assert_int_equal(resp.status_code, HTTP_STATUS_INTERNAL_ERROR);
    httpResponseFree(&resp);
}

/**
 * @brief Verifies GET /notes returns list and captures tag filter.
 */
static void testListNotesSuccess(void **state)
{
    (void)state;
    resetMocks();

    g_list_out.count = 2;
    g_list_out.items = calloc(2, sizeof(Note));
    fillNote(&g_list_out.items[0], "id-1", "T1", "C1", "home");
    fillNote(&g_list_out.items[1], "id-2", "T2", "C2", "work");

    NotesHandlerContext ctx = { .service = (NotesService *)0x1 };
    HttpRequest req = { .method = "GET", .url = "/notes", .path = "/notes" };

    HttpResponse resp = listNotesHandler(&req, "work", &ctx);
    assert_int_equal(resp.status_code, HTTP_STATUS_OK);
    assert_string_equal(g_last_tag, "work");

    json_error_t error;
    json_t *json = json_loads(resp.body, 0, &error);
    assert_non_null(json);
    assert_true(json_is_array(json));
    assert_int_equal(json_array_size(json), 2);

    json_decref(json);
    httpResponseFree(&resp);
}

/**
 * @brief Verifies GET /notes/{id} returns note and captures id.
 */
static void testGetNoteSuccess(void **state)
{
    (void)state;
    resetMocks();

    fillNote(&g_get_out, "id-1", "T1", "C1", "x");

    NotesHandlerContext ctx = { .service = (NotesService *)0x1 };
    HttpRequest req = { .method = "GET", .url = "/notes/id-1", .path = "/notes/id-1" };

    HttpResponse resp = getNoteHandler(&req, "id-1", &ctx);
    assert_int_equal(resp.status_code, HTTP_STATUS_OK);
    assert_string_equal(g_last_id, "id-1");
    httpResponseFree(&resp);
    noteFree(&g_get_out);
}

/**
 * @brief Verifies GET /notes/{id} propagates not-found to 404.
 */
static void testGetNoteServiceError(void **state)
{
    (void)state;
    resetMocks();

    g_ret_get = RETCODE_NOTES_SERVICE_NOT_FOUND;

    NotesHandlerContext ctx = { .service = (NotesService *)0x1 };
    HttpRequest req = { .method = "GET", .url = "/notes/id-1", .path = "/notes/id-1" };

    HttpResponse resp = getNoteHandler(&req, "id-1", &ctx);
    assert_int_equal(resp.status_code, HTTP_STATUS_NOT_FOUND);
    httpResponseFree(&resp);
}

/**
 * @brief Verifies PUT /notes/{id} updates note and returns 200.
 */
static void testUpdateNoteSuccess(void **state)
{
    (void)state;
    resetMocks();

    fillNote(&g_update_out, "id-1", "T1", "C1", "x");

    NotesHandlerContext ctx = { .service = (NotesService *)0x1 };
    const char *body = "{\"title\":\"New\",\"content\":\"C\",\"tags\":[\"x\"]}";
    HttpRequest req = {
        .method = "PUT",
        .url = "/notes/id-1",
        .path = "/notes/id-1",
        .body = body,
        .body_size = strlen(body),
    };

    HttpResponse resp = updateNoteHandler(&req, "id-1", &ctx);
    assert_int_equal(resp.status_code, HTTP_STATUS_OK);
    assert_string_equal(g_last_id, "id-1");
    assert_true(g_last_input_set);
    assert_string_equal(g_last_input.title, "New");

    httpResponseFree(&resp);
    noteFree(&g_update_out);
}

/**
 * @brief Verifies invalid PUT /notes/{id} body returns 400.
 */
static void testUpdateNoteInvalidBody(void **state)
{
    (void)state;
    resetMocks();

    NotesHandlerContext ctx = { .service = (NotesService *)0x1 };
    const char *body = "{\"title\":}";
    HttpRequest req = {
        .method = "PUT",
        .url = "/notes/id-1",
        .path = "/notes/id-1",
        .body = body,
        .body_size = strlen(body),
    };

    HttpResponse resp = updateNoteHandler(&req, "id-1", &ctx);
    assert_int_equal(resp.status_code, HTTP_STATUS_BAD_REQUEST);
    httpResponseFree(&resp);
}

/**
 * @brief Verifies update service error maps to 500.
 */
static void testUpdateNoteServiceError(void **state)
{
    (void)state;
    resetMocks();

    g_ret_update = RETCODE_NOTES_SERVICE_NO_MEMORY;

    NotesHandlerContext ctx = { .service = (NotesService *)0x1 };
    const char *body = "{\"title\":\"New\",\"content\":\"C\"}";
    HttpRequest req = {
        .method = "PUT",
        .url = "/notes/id-1",
        .path = "/notes/id-1",
        .body = body,
        .body_size = strlen(body),
    };

    HttpResponse resp = updateNoteHandler(&req, "id-1", &ctx);
    assert_int_equal(resp.status_code, HTTP_STATUS_INTERNAL_ERROR);
    httpResponseFree(&resp);
}

/**
 * @brief Verifies DELETE /notes/{id} returns 200 and status payload.
 */
static void testDeleteNoteSuccess(void **state)
{
    (void)state;
    resetMocks();

    NotesHandlerContext ctx = { .service = (NotesService *)0x1 };
    HttpRequest req = { .method = "DELETE", .url = "/notes/id-1", .path = "/notes/id-1" };

    HttpResponse resp = deleteNoteHandler(&req, "id-1", &ctx);
    assert_int_equal(resp.status_code, HTTP_STATUS_OK);
    assert_string_equal(g_last_id, "id-1");

    json_t *json = json_loads(resp.body, 0, NULL);
    assert_non_null(json);
    json_t *status = json_object_get(json, "status");
    assert_true(json_is_string(status));
    assert_string_equal(json_string_value(status), "deleted");

    json_decref(json);
    httpResponseFree(&resp);
}

/**
 * @brief Verifies delete service error maps to 500.
 */
static void testDeleteNoteServiceError(void **state)
{
    (void)state;
    resetMocks();

    g_ret_delete = RETCODE_NOTES_SERVICE_NO_MEMORY;

    NotesHandlerContext ctx = { .service = (NotesService *)0x1 };
    HttpRequest req = { .method = "DELETE", .url = "/notes/id-1", .path = "/notes/id-1" };

    HttpResponse resp = deleteNoteHandler(&req, "id-1", &ctx);
    assert_int_equal(resp.status_code, HTTP_STATUS_INTERNAL_ERROR);
    httpResponseFree(&resp);
}

/**
 * @brief Verifies list service error maps to 500.
 */
static void testListNotesServiceError(void **state)
{
    (void)state;
    resetMocks();

    g_ret_list = RETCODE_NOTES_SERVICE_STORAGE_ERROR;

    NotesHandlerContext ctx = { .service = (NotesService *)0x1 };
    HttpRequest req = { .method = "GET", .url = "/notes", .path = "/notes" };

    HttpResponse resp = listNotesHandler(&req, NULL, &ctx);
    assert_int_equal(resp.status_code, HTTP_STATUS_INTERNAL_ERROR);
    httpResponseFree(&resp);
}

/**
 * @brief Verifies missing handler context returns 500.
 */
static void testHandlerMissingContext(void **state)
{
    (void)state;
    HttpRequest req = { .method = "GET", .url = "/notes", .path = "/notes" };

    HttpResponse resp = listNotesHandler(&req, NULL, NULL);
    assert_int_equal(resp.status_code, HTTP_STATUS_INTERNAL_ERROR);
    httpResponseFree(&resp);
}

/**
 * @brief Verifies create/destroy context lifecycle with NULL guard.
 */
static void testNotesHandlerContextLifecycle(void **state)
{
    (void)state;
    assert_null(createNotesHandlerContext(NULL));

    NotesHandlerContext *ctx = createNotesHandlerContext((NotesService *)0x1);
    assert_non_null(ctx);
    assert_int_equal(destroyNotesHandlerContext(ctx), RETCODE_OK);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(testCreateNoteSuccess),       cmocka_unit_test(testCreateNoteInvalidBody),
        cmocka_unit_test(testCreateNoteServiceError),  cmocka_unit_test(testListNotesSuccess),
        cmocka_unit_test(testListNotesServiceError),   cmocka_unit_test(testGetNoteSuccess),
        cmocka_unit_test(testGetNoteServiceError),     cmocka_unit_test(testUpdateNoteSuccess),
        cmocka_unit_test(testUpdateNoteInvalidBody),   cmocka_unit_test(testUpdateNoteServiceError),
        cmocka_unit_test(testDeleteNoteSuccess),       cmocka_unit_test(testDeleteNoteServiceError),
        cmocka_unit_test(testHandlerMissingContext),   cmocka_unit_test(testNotesHandlerContextLifecycle),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
