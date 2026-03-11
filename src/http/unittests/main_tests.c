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
        cmocka_unit_test(test_response_free_resets_fields),
        cmocka_unit_test(test_router_routes_collection_get_with_tag),
        cmocka_unit_test(test_router_decodes_tag_value),
        cmocka_unit_test(test_router_routes_item_get),
        cmocka_unit_test(test_router_routes_item_put),
        cmocka_unit_test(test_router_routes_item_delete),
        cmocka_unit_test(test_router_rejects_unknown_path),
        cmocka_unit_test(test_router_missing_handler_returns_500),
        cmocka_unit_test(test_router_method_not_allowed),
        cmocka_unit_test(test_server_create_destroy),
        cmocka_unit_test(test_server_handles_get_with_query),
        cmocka_unit_test(test_server_handles_put_with_body),
        cmocka_unit_test(test_server_create_invalid_args),
        cmocka_unit_test(test_server_start_null),
        cmocka_unit_test(test_server_stop_without_start),
        cmocka_unit_test(test_server_destroy_null),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
