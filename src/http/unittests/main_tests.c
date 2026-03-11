/**
 * @file main_tests.c
 * @brief Test runner for HTTP unit tests.
 */
#include "api/router.h"
#include "api/response.h"
#include "api/server.h"
#include "api/json_utils.h"
#include "main_tests.h"

#include <string.h>
#include <setjmp.h>
#include <cmocka.h>

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(testResponseFreeResetsFields),
        cmocka_unit_test(testRouterRoutesCollectionGetWithTag),
        cmocka_unit_test(testRouterDecodesTagValue),
        cmocka_unit_test(testRouterRoutesItemGet),
        cmocka_unit_test(testRouterRoutesItemPut),
        cmocka_unit_test(testRouterRoutesItemDelete),
        cmocka_unit_test(testRouterRejectsUnknownPath),
        cmocka_unit_test(testRouterMissingHandlerReturns500),
        cmocka_unit_test(testRouterMethodNotAllowed),
        cmocka_unit_test(testServerCreateDestroy),
        cmocka_unit_test(testServerHandlesGetWithQuery),
        cmocka_unit_test(testServerHandlesPutWithBody),
        cmocka_unit_test(testServerCreateInvalidArgs),
        cmocka_unit_test(testServerStartNull),
        cmocka_unit_test(testServerStopWithoutStart),
        cmocka_unit_test(testServerDestroyNull),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
