#ifndef HTTP_MAIN_TESTS_H
#define HTTP_MAIN_TESTS_H

void test_response_free_resets_fields();
void test_router_routes_collection_get_with_tag();
void test_router_decodes_tag_value();
void test_router_routes_item_get();
void test_router_routes_item_put();
void test_router_routes_item_delete();
void test_router_rejects_unknown_path();
void test_router_missing_handler_returns_500();
void test_router_method_not_allowed();
void test_server_create_destroy();
void test_server_handles_get_with_query();
void test_server_handles_put_with_body();
void test_server_create_invalid_args();
void test_server_start_null();
void test_server_stop_without_start();
void test_server_destroy_null();

#endif /* HTTP_MAIN_TESTS_H */
