/**
 * @file router_tests.c
 * @brief Unit tests for HTTP router behavior.
 */
#include "api/router.h"
#include "api/response.h"
#include "api/server.h"
#include "api/json_utils.h"
#include "mock_functions.h"
#include "main_tests.h"

#include <string.h>
#include <setjmp.h>
#include <cmocka.h>

/**
 * @brief Routes GET /notes with a tag filter to list handler.
 */
void testRouterRoutesCollectionGetWithTag(void **state)
{
    (void)state;
    HandlerCapture cap;
    resetCapture(&cap);

    HttpRouterConfig config = {
        .create_note = handleCreate,
        .list_notes = handleList,
        .get_note = handleGet,
        .update_note = handleUpdate,
        .delete_note = handleDelete,
        .notes_user_data = &cap,
    };

    HttpRouter *router = httpRouterCreate(&config);
    assert_non_null(router);

    HttpRequest req = { 0 };
    req.method = "GET";
    req.url = "/notes?tag=work";
    req.path = "/notes";
    req.query = "tag=work";

    HttpResponse resp = httpRouterHandle(&req, router);
    assert_int_equal(resp.status_code, HTTP_STATUS_OK);
    assert_int_equal(cap.called_list, 1);
    assert_string_equal(cap.tag, "work");

    httpResponseFree(&resp);
    assert_int_equal(httpRouterDestroy(router), RETCODE_OK);
}

/**
 * @brief Decodes URL-encoded tag values.
 */
void testRouterDecodesTagValue(void **state)
{
    (void)state;
    HandlerCapture cap;
    resetCapture(&cap);

    HttpRouterConfig config = {
        .list_notes = handleList,
        .notes_user_data = &cap,
    };

    HttpRouter *router = httpRouterCreate(&config);
    assert_non_null(router);

    HttpRequest req = { 0 };
    req.method = "GET";
    req.url = "/notes?tag=foo%20bar";
    req.path = "/notes";
    req.query = "tag=foo%20bar";

    HttpResponse resp = httpRouterHandle(&req, router);
    assert_int_equal(resp.status_code, HTTP_STATUS_OK);
    assert_string_equal(cap.tag, "foo bar");

    httpResponseFree(&resp);
    assert_int_equal(httpRouterDestroy(router), RETCODE_OK);
}

/**
 * @brief Routes GET /notes/{id} to get handler.
 */
void testRouterRoutesItemGet(void **state)
{
    (void)state;
    HandlerCapture cap;
    resetCapture(&cap);

    HttpRouterConfig config = {
        .get_note = handleGet,
        .notes_user_data = &cap,
    };

    HttpRouter *router = httpRouterCreate(&config);
    assert_non_null(router);

    HttpRequest req = { 0 };
    req.method = "GET";
    req.url = "/notes/abc123";
    req.path = "/notes/abc123";

    HttpResponse resp = httpRouterHandle(&req, router);
    assert_int_equal(resp.status_code, HTTP_STATUS_OK);
    assert_int_equal(cap.called_get, 1);
    assert_string_equal(cap.note_id, "abc123");

    httpResponseFree(&resp);
    assert_int_equal(httpRouterDestroy(router), RETCODE_OK);
}

/**
 * @brief Routes PUT /notes/{id} to update handler.
 */
void testRouterRoutesItemPut(void **state)
{
    (void)state;
    HandlerCapture cap;
    resetCapture(&cap);

    HttpRouterConfig config = {
        .update_note = handleUpdate,
        .notes_user_data = &cap,
    };

    HttpRouter *router = httpRouterCreate(&config);
    assert_non_null(router);

    HttpRequest req = { 0 };
    req.method = "PUT";
    req.url = "/notes/abc123";
    req.path = "/notes/abc123";

    HttpResponse resp = httpRouterHandle(&req, router);
    assert_int_equal(resp.status_code, HTTP_STATUS_OK);
    assert_int_equal(cap.called_update, 1);
    assert_string_equal(cap.note_id, "abc123");

    httpResponseFree(&resp);
    assert_int_equal(httpRouterDestroy(router), RETCODE_OK);
}

/**
 * @brief Routes DELETE /notes/{id} to delete handler.
 */
void testRouterRoutesItemDelete(void **state)
{
    (void)state;
    HandlerCapture cap;
    resetCapture(&cap);

    HttpRouterConfig config = {
        .delete_note = handleDelete,
        .notes_user_data = &cap,
    };

    HttpRouter *router = httpRouterCreate(&config);
    assert_non_null(router);

    HttpRequest req = { 0 };
    req.method = "DELETE";
    req.url = "/notes/abc123";
    req.path = "/notes/abc123";

    HttpResponse resp = httpRouterHandle(&req, router);
    assert_int_equal(resp.status_code, HTTP_STATUS_OK);
    assert_int_equal(cap.called_delete, 1);
    assert_string_equal(cap.note_id, "abc123");

    httpResponseFree(&resp);
    assert_int_equal(httpRouterDestroy(router), RETCODE_OK);
}

/**
 * @brief Rejects unknown paths with 404.
 */
void testRouterRejectsUnknownPath(void **state)
{
    (void)state;
    HandlerCapture cap;
    resetCapture(&cap);

    HttpRouterConfig config = {
        .list_notes = handleList,
        .notes_user_data = &cap,
    };

    HttpRouter *router = httpRouterCreate(&config);
    assert_non_null(router);

    HttpRequest req = { 0 };
    req.method = "GET";
    req.url = "/note";
    req.path = "/note";

    HttpResponse resp = httpRouterHandle(&req, router);
    assert_int_equal(resp.status_code, HTTP_STATUS_NOT_FOUND);

    httpResponseFree(&resp);
    assert_int_equal(httpRouterDestroy(router), RETCODE_OK);
}

/**
 * @brief Missing handler returns 500.
 */
void testRouterMissingHandlerReturns500(void **state)
{
    (void)state;
    HttpRouterConfig config = { 0 };
    HttpRouter *router = httpRouterCreate(&config);
    assert_non_null(router);

    HttpRequest req = { 0 };
    req.method = "GET";
    req.url = "/notes";
    req.path = "/notes";

    HttpResponse resp = httpRouterHandle(&req, router);
    assert_int_equal(resp.status_code, HTTP_STATUS_INTERNAL_ERROR);

    httpResponseFree(&resp);
    assert_int_equal(httpRouterDestroy(router), RETCODE_OK);
}

/**
 * @brief Unsupported method returns 405.
 */
void testRouterMethodNotAllowed(void **state)
{
    (void)state;
    HandlerCapture cap;
    resetCapture(&cap);

    HttpRouterConfig config = {
        .list_notes = handleList,
        .notes_user_data = &cap,
    };

    HttpRouter *router = httpRouterCreate(&config);
    assert_non_null(router);

    HttpRequest req = { 0 };
    req.method = "PUT";
    req.url = "/notes";
    req.path = "/notes";

    HttpResponse resp = httpRouterHandle(&req, router);
    assert_int_equal(resp.status_code, HTTP_STATUS_METHOD_NOT_ALLOWED);

    httpResponseFree(&resp);
    assert_int_equal(httpRouterDestroy(router), RETCODE_OK);
}
