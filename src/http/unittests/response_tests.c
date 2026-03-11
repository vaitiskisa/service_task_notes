#include "api/router.h"
/**
 * @file response_tests.c
 * @brief Unit tests for HTTP response helpers.
 */
#include "api/response.h"
#include "api/server.h"
#include "api/json_utils.h"
#include "mock_functions.h"
#include "main_tests.h"

#include <string.h>
#include <setjmp.h>
#include <cmocka.h>

/**
 * @brief Verifies httpResponseFree clears all fields.
 */
void testResponseFreeResetsFields(void **state)
{
    (void)state;
    HttpResponse response = { 0 };
    response.status_code = 200;
    response.content_type = strdupSafe("application/json");
    response.body = strdupSafe("{}");
    response.body_size = response.body ? strlen(response.body) : 0;

    assert_int_equal(httpResponseFree(&response), RETCODE_OK);
    assert_null(response.content_type);
    assert_null(response.body);
    assert_int_equal(response.body_size, 0);
    assert_int_equal(response.status_code, 0);
}
