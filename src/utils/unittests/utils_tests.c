/**
 * @file utils_tests.c
 * @brief Unit tests for JSON and response utilities.
 */
#include "api/json_utils.h"
#include "api/response.h"

#include <jansson.h>
#include <string.h>
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>

/**
 * @brief Verifies strdupSafe duplicates strings and handles NULL.
 */
static void testStrdupSafe(void **state)
{
    (void)state;
    char *copy = strdupSafe("hello");
    assert_non_null(copy);
    assert_string_equal(copy, "hello");
    free(copy);

    assert_null(strdupSafe(NULL));
}

/**
 * @brief Verifies makeJsonStringResponse defaults body to "{}" on NULL.
 */
static void testMakeJsonStringResponseDefaults(void **state)
{
    (void)state;
    HttpResponse resp = makeJsonStringResponse(HTTP_STATUS_OK, NULL);
    assert_int_equal(resp.status_code, HTTP_STATUS_OK);
    assert_non_null(resp.content_type);
    assert_string_equal(resp.content_type, HTTP_CONTENT_TYPE_JSON);
    assert_non_null(resp.body);
    assert_string_equal(resp.body, "{}");

    httpResponseFree(&resp);
}

/**
 * @brief Verifies makeJsonObjectResponse serializes JSON successfully.
 */
static void testMakeJsonObjectResponseSuccess(void **state)
{
    (void)state;
    json_t *obj = json_object();
    assert_non_null(obj);
    json_object_set_new(obj, "k", json_string("v"));

    HttpResponse resp = makeJsonObjectResponse(HTTP_STATUS_CREATED, obj);
    assert_int_equal(resp.status_code, HTTP_STATUS_CREATED);
    assert_non_null(resp.content_type);
    assert_string_equal(resp.content_type, HTTP_CONTENT_TYPE_JSON);
    assert_non_null(resp.body);

    json_error_t error;
    json_t *parsed = json_loads(resp.body, 0, &error);
    assert_non_null(parsed);
    json_t *val = json_object_get(parsed, "k");
    assert_true(json_is_string(val));
    assert_string_equal(json_string_value(val), "v");

    json_decref(parsed);
    httpResponseFree(&resp);
}

/**
 * @brief Verifies makeErrorResponse returns JSON error payload.
 */
static void testMakeErrorResponse(void **state)
{
    (void)state;
    HttpResponse resp = makeErrorResponse(HTTP_STATUS_BAD_REQUEST, "bad");
    assert_int_equal(resp.status_code, HTTP_STATUS_BAD_REQUEST);
    assert_non_null(resp.body);

    json_t *json = json_loads(resp.body, 0, NULL);
    assert_non_null(json);
    json_t *val = json_object_get(json, "error");
    assert_true(json_is_string(val));
    assert_string_equal(json_string_value(val), "bad");

    json_decref(json);
    httpResponseFree(&resp);
}

/**
 * @brief Verifies parseJsonRequestBody parses valid JSON.
 */
static void testParseJsonRequestBody(void **state)
{
    (void)state;
    const char *body = "{\"a\":1}";
    HttpRequest req = {
        .body = body,
        .body_size = strlen(body),
    };

    json_t *json = NULL;
    assert_int_equal(parseJsonRequestBody(&req, &json), 0);
    assert_non_null(json);
    json_t *val = json_object_get(json, "a");
    assert_true(json_is_integer(val));
    assert_int_equal((int)json_integer_value(val), 1);
    json_decref(json);
}

/**
 * @brief Verifies parseJsonRequestBody rejects invalid JSON.
 */
static void testParseJsonRequestBodyInvalid(void **state)
{
    (void)state;
    HttpRequest req = { 0 };
    json_t *json = (json_t *)0x1;
    assert_int_equal(parseJsonRequestBody(&req, &json), -1);
    assert_null(json);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(testStrdupSafe),
        cmocka_unit_test(testMakeJsonStringResponseDefaults),
        cmocka_unit_test(testMakeJsonObjectResponseSuccess),
        cmocka_unit_test(testMakeErrorResponse),
        cmocka_unit_test(testParseJsonRequestBody),
        cmocka_unit_test(testParseJsonRequestBodyInvalid),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
