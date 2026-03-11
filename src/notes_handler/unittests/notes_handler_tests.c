#include "api/notes_handler.h"
#include "api/json_utils.h"
#include "mock_functions.h"

#include <jansson.h>
#include <string.h>
#include <setjmp.h>
#include <cmocka.h>

static void test_create_note_success(void **state)
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

static void test_create_note_invalid_body(void **state)
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

static void test_create_note_service_error(void **state)
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

static void test_list_notes_success(void **state)
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

static void test_get_note_success(void **state)
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

static void test_get_note_service_error(void **state)
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

static void test_update_note_success(void **state)
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

static void test_update_note_invalid_body(void **state)
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

static void test_update_note_service_error(void **state)
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

static void test_delete_note_success(void **state)
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

static void test_delete_note_service_error(void **state)
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

static void test_list_notes_service_error(void **state)
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

static void test_handler_missing_context(void **state)
{
    (void)state;
    HttpRequest req = { .method = "GET", .url = "/notes", .path = "/notes" };

    HttpResponse resp = listNotesHandler(&req, NULL, NULL);
    assert_int_equal(resp.status_code, HTTP_STATUS_INTERNAL_ERROR);
    httpResponseFree(&resp);
}

static void test_notes_handler_context_lifecycle(void **state)
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
        cmocka_unit_test(test_create_note_success),       cmocka_unit_test(test_create_note_invalid_body),
        cmocka_unit_test(test_create_note_service_error), cmocka_unit_test(test_list_notes_success),
        cmocka_unit_test(test_list_notes_service_error),  cmocka_unit_test(test_get_note_success),
        cmocka_unit_test(test_get_note_service_error),    cmocka_unit_test(test_update_note_success),
        cmocka_unit_test(test_update_note_invalid_body),  cmocka_unit_test(test_update_note_service_error),
        cmocka_unit_test(test_delete_note_success),       cmocka_unit_test(test_delete_note_service_error),
        cmocka_unit_test(test_handler_missing_context),   cmocka_unit_test(test_notes_handler_context_lifecycle),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
