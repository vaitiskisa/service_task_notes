#include "api/router.h"
#include "api/response.h"
#include "api/server.h"
#include "api/json_utils.h"
#include "mock_functions.h"
#include "main_tests.h"

#include <string.h>
#include <stdlib.h>
#include <setjmp.h>
#include <cmocka.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct ServerCapture {
    char method[16];
    char path[128];
    char query[256];
    char body[2048];
    size_t body_size;
    int called;
} ServerCapture;

static HttpResponse captureHandler(const HttpRequest *request, void *user_data)
{
    ServerCapture *cap = (ServerCapture *)user_data;
    cap->called = 1;

    if(request && request->method) {
        snprintf(cap->method, sizeof(cap->method), "%s", request->method);
    }
    if(request && request->path) {
        snprintf(cap->path, sizeof(cap->path), "%s", request->path);
    }
    if(request && request->query) {
        snprintf(cap->query, sizeof(cap->query), "%s", request->query);
    }
    if(request && request->body && request->body_size > 0) {
        size_t copy_len = request->body_size;
        if(copy_len >= sizeof(cap->body)) {
            copy_len = sizeof(cap->body) - 1;
        }
        memcpy(cap->body, request->body, copy_len);
        cap->body[copy_len] = '\0';
        cap->body_size = request->body_size;
    }

    return makeJsonStringResponse(HTTP_STATUS_OK, "{\"ok\":true}");
}

static uint16_t findFreePort(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        return 0;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(fd);
        return 0;
    }

    socklen_t len = sizeof(addr);
    if(getsockname(fd, (struct sockaddr *)&addr, &len) != 0) {
        close(fd);
        return 0;
    }

    uint16_t port = ntohs(addr.sin_port);
    close(fd);
    return port;
}

static int waitForPort(uint16_t port)
{
    for(int i = 0; i < 50; i++) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if(fd < 0) {
            return -1;
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        int rc = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
        close(fd);
        if(rc == 0) {
            return 0;
        }
        usleep(20000);
    }

    return -1;
}

static int sendHttpRequest(uint16_t port, const char *method, const char *path, const char *body, char *out,
                           size_t out_cap)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if(connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(fd);
        return -1;
    }

    const char *req_body = body ? body : "";
    size_t body_len = strlen(req_body);

    char header[512];
    int header_len = 0;

    if(body_len > 0) {
        header_len = snprintf(header, sizeof(header),
                              "%s %s HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/json\r\n"
                              "Content-Length: %zu\r\nConnection: close\r\n\r\n",
                              method, path, body_len);
    } else {
        header_len = snprintf(header, sizeof(header),
                              "%s %s HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n", method, path);
    }

    if(header_len <= 0 || (size_t)header_len >= sizeof(header)) {
        close(fd);
        return -1;
    }

    ssize_t written = send(fd, header, (size_t)header_len, 0);
    if(written != header_len) {
        close(fd);
        return -1;
    }

    if(body_len > 0) {
        written = send(fd, req_body, body_len, 0);
        if(written != (ssize_t)body_len) {
            close(fd);
            return -1;
        }
    }

    if(out && out_cap > 0) {
        size_t total = 0;
        ssize_t n = 0;
        while((n = recv(fd, out + total, out_cap - 1 - total, 0)) > 0) {
            total += (size_t)n;
            if(total >= out_cap - 1) {
                break;
            }
        }
        out[total] = '\0';
    }

    close(fd);
    return 0;
}

void test_server_create_destroy(void **state)
{
    (void)state;
    HttpServerConfig invalid = { 0 };
    assert_null(httpServerCreate(&invalid));

    HttpServerConfig valid = { .port = 8080, .handler = handleNoop, .handler_user_data = NULL };
    HttpServer *server = httpServerCreate(&valid);
    assert_non_null(server);
    assert_int_equal(httpServerDestroy(server), RETCODE_OK);
}

void test_server_handles_get_with_query(void **state)
{
    (void)state;
    ServerCapture cap = { 0 };

    uint16_t port = findFreePort();
    assert_true(port > 0);

    HttpServerConfig cfg = { .port = port, .handler = captureHandler, .handler_user_data = &cap };
    HttpServer *server = httpServerCreate(&cfg);
    assert_non_null(server);
    assert_int_equal(httpServerStart(server), RETCODE_OK);
    assert_int_equal(waitForPort(port), 0);

    char response[1024] = { 0 };
    assert_int_equal(sendHttpRequest(port, "GET", "/notes?tag=work&x=1", NULL, response, sizeof(response)), 0);
    assert_non_null(strstr(response, "200"));

    assert_int_equal(cap.called, 1);
    assert_string_equal(cap.method, "GET");
    assert_string_equal(cap.path, "/notes");
    assert_string_equal(cap.query, "tag=work&x=1");

    assert_int_equal(httpServerStop(server), RETCODE_OK);
    assert_int_equal(httpServerDestroy(server), RETCODE_OK);
}

void test_server_handles_put_with_body(void **state)
{
    (void)state;
    ServerCapture cap = { 0 };

    uint16_t port = findFreePort();
    assert_true(port > 0);

    HttpServerConfig cfg = { .port = port, .handler = captureHandler, .handler_user_data = &cap };
    HttpServer *server = httpServerCreate(&cfg);
    assert_non_null(server);
    assert_int_equal(httpServerStart(server), RETCODE_OK);
    assert_int_equal(waitForPort(port), 0);

    char big_body[1500];
    memset(big_body, 'a', sizeof(big_body) - 1);
    big_body[sizeof(big_body) - 1] = '\0';

    char response[1024] = { 0 };
    assert_int_equal(sendHttpRequest(port, "PUT", "/notes/abc123", big_body, response, sizeof(response)), 0);
    assert_non_null(strstr(response, "200"));

    assert_int_equal(cap.called, 1);
    assert_string_equal(cap.method, "PUT");
    assert_string_equal(cap.path, "/notes/abc123");
    assert_int_equal((int)cap.body_size, (int)strlen(big_body));
    assert_int_equal(memcmp(cap.body, big_body, 32), 0);

    assert_int_equal(httpServerStop(server), RETCODE_OK);
    assert_int_equal(httpServerDestroy(server), RETCODE_OK);
}

void test_server_create_invalid_args(void **state)
{
    (void)state;
    assert_null(httpServerCreate(NULL));

    HttpServerConfig no_handler = { .port = 8080, .handler = NULL, .handler_user_data = NULL };
    assert_null(httpServerCreate(&no_handler));

    HttpServerConfig no_port = { .port = 0, .handler = handleNoop, .handler_user_data = NULL };
    assert_null(httpServerCreate(&no_port));
}

void test_server_start_null(void **state)
{
    (void)state;
    assert_int_equal(httpServerStart(NULL), RETCODE_COMMON_NULL_ARG);
}

void test_server_stop_without_start(void **state)
{
    (void)state;
    HttpServerConfig valid = { .port = 8080, .handler = handleNoop, .handler_user_data = NULL };
    HttpServer *server = httpServerCreate(&valid);
    assert_non_null(server);

    assert_int_equal(httpServerStop(server), RETCODE_COMMON_NULL_ARG);
    assert_int_equal(httpServerDestroy(server), RETCODE_OK);
}

void test_server_destroy_null(void **state)
{
    (void)state;
    assert_int_equal(httpServerDestroy(NULL), RETCODE_COMMON_NOT_INITIALIZED);
}
