/**
 * @file server.c
 * @brief HTTP server implementation using libmicrohttpd.
 */
#include "api/server.h"
#include "api/request.h"
#include "api/response.h"
#include "api/json_utils.h"

#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct RequestContext {
    /** Accumulated request body buffer. */
    char *body;
    /** Current size of @ref body. */
    size_t body_size;
    /** Allocated capacity of @ref body. */
    size_t body_capacity;
} RequestContext;

struct HttpServer {
    /** TCP port. */
    uint16_t port;
    /** Request handler callback. */
    HttpRouteHandler handler;
    /** User data passed to the handler. */
    void *handler_user_data;
    /** libmicrohttpd daemon handle. */
    struct MHD_Daemon *daemon;
};

static RetCode requestContextFree(RequestContext *ctx)
{
    RETURN_ON_COND(!ctx, RETCODE_COMMON_NULL_ARG);

    free(ctx->body);
    free(ctx);

    return RETCODE_OK;
}

static RetCode requestContextAppendBody(RequestContext *ctx, const char *data, size_t size)
{
    RETURN_ON_COND(!ctx, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(!data, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(size == 0, RETCODE_COMMON_INVALID_ARG);

    if(ctx->body_size + size + 1 > ctx->body_capacity) {
        size_t new_capacity = (ctx->body_capacity == 0) ? 1024 : ctx->body_capacity;

        while(new_capacity < ctx->body_size + size + 1) {
            new_capacity *= 2;
        }

        char *new_body = realloc(ctx->body, new_capacity);
        RETURN_ON_COND(!new_body, RETCODE_COMMON_NO_MEMORY);

        ctx->body = new_body;
        ctx->body_capacity = new_capacity;
    }

    memcpy(ctx->body + ctx->body_size, data, size);
    ctx->body_size += size;
    ctx->body[ctx->body_size] = '\0';

    return RETCODE_OK;
}

static enum MHD_Result sendResponse(struct MHD_Connection *connection, const HttpResponse *response)
{
    struct MHD_Response *mhd_response =
            MHD_create_response_from_buffer(response->body_size, (void *)response->body, MHD_RESPMEM_MUST_COPY);

    if(!mhd_response) {
        return MHD_NO;
    }

    if(response->content_type) {
        MHD_add_response_header(mhd_response, "Content-Type", response->content_type);
    }

    enum MHD_Result ret = MHD_queue_response(connection, response->status_code, mhd_response);
    MHD_destroy_response(mhd_response);

    return ret;
}

static const char *getQueryString(const char *url)
{
    const char *q = strchr(url, '?');
    return q ? (q + 1) : NULL;
}

typedef struct QueryBuilder {
    char *buf;
    size_t len;
    size_t cap;
} QueryBuilder;

static enum MHD_Result queryBuilderAppend(QueryBuilder *qb, const char *key, const char *value)
{
    if(!qb || !key) {
        return MHD_YES;
    }

    const char *val = value ? value : "";
    size_t key_len = strlen(key);
    size_t val_len = strlen(val);
    size_t extra = key_len + (val_len ? (1 + val_len) : 0) + (qb->len ? 1 : 0) + 1;

    if(qb->len + extra > qb->cap) {
        size_t new_cap = (qb->cap == 0) ? 128 : qb->cap;
        while(new_cap < qb->len + extra) {
            new_cap *= 2;
        }

        char *new_buf = realloc(qb->buf, new_cap);
        if(!new_buf) {
            return MHD_NO;
        }

        qb->buf = new_buf;
        qb->cap = new_cap;
    }

    if(qb->len) {
        qb->buf[qb->len++] = '&';
    }

    memcpy(qb->buf + qb->len, key, key_len);
    qb->len += key_len;

    if(val_len) {
        qb->buf[qb->len++] = '=';
        memcpy(qb->buf + qb->len, val, val_len);
        qb->len += val_len;
    }

    qb->buf[qb->len] = '\0';
    return MHD_YES;
}

static enum MHD_Result queryValueIterator(void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
    (void)kind;
    return queryBuilderAppend((QueryBuilder *)cls, key, value);
}

static char *buildQueryString(struct MHD_Connection *connection)
{
    QueryBuilder qb = { 0 };
    int count = MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, queryValueIterator, &qb);
    if(count <= 0) {
        free(qb.buf);
        return NULL;
    }

    return qb.buf;
}

static char *extractPath(const char *url)
{
    const char *q = strchr(url, '?');
    size_t len = q ? (size_t)(q - url) : strlen(url);

    char *path = malloc(len + 1);
    if(!path) {
        return NULL;
    }

    memcpy(path, url, len);
    path[len] = '\0';
    return path;
}

static enum MHD_Result accessHandlerCallback(void *cls, struct MHD_Connection *connection, const char *url,
                                             const char *method, const char *version, const char *upload_data,
                                             size_t *upload_data_size, void **con_cls)
{
    RetCode ret_code = RETCODE_OK;

    (void)version;

    HttpServer *server = (HttpServer *)cls;
    RequestContext *ctx = (RequestContext *)(*con_cls);

    if(!ctx) {
        ctx = calloc(1, sizeof(*ctx));
        if(!ctx) {
            return MHD_NO;
        }

        *con_cls = ctx;
        return MHD_YES;
    }

    if(*upload_data_size > 0) {
        ret_code = requestContextAppendBody(ctx, upload_data, *upload_data_size);
        if(ret_code != RETCODE_OK) {
            return MHD_NO;
        }

        *upload_data_size = 0;
        return MHD_YES;
    }

    HttpRequest request = { 0 };
    request.method = method;
    request.url = url;
    request.path = extractPath(url);
    request.query = getQueryString(url);
    if(!request.query) {
        /* Fallback in case the URL does not contain a query string */
        request.query = buildQueryString(connection);
    }
    request.body = ctx->body;
    request.body_size = ctx->body_size;
    request.headers = NULL;
    request.header_count = 0;

    if(!request.path) {
        free((void *)request.path);
        LOG_ON_ERROR(requestContextFree(ctx));
        *con_cls = NULL;
        return MHD_NO;
    }

    HttpResponse response = { 0 };

    if(server->handler) {
        response = server->handler(&request, server->handler_user_data);
    } else {
        response.status_code = 500;
        response.content_type = strdupSafe(HTTP_CONTENT_TYPE_JSON);
        response.body = strdupSafe("{\"error\":\"No route handler configured\"}");
        response.body_size = response.body ? strlen(response.body) : 0;
    }

    enum MHD_Result ret = sendResponse(connection, &response);

    httpResponseFree(&response);
    free((void *)request.path);
    if(request.query && request.query != getQueryString(url)) {
        free((void *)request.query);
    }
    LOG_ON_ERROR(requestContextFree(ctx));
    *con_cls = NULL;

    return ret;
}

HttpServer *httpServerCreate(const HttpServerConfig *config)
{
    if(!config || !config->handler || config->port == 0) {
        return NULL;
    }

    HttpServer *server = calloc(1, sizeof(*server));
    if(!server) {
        return NULL;
    }

    server->port = config->port;
    server->handler = config->handler;
    server->handler_user_data = config->handler_user_data;
    server->daemon = NULL;

    return server;
}

RetCode httpServerStart(HttpServer *server)
{
    RETURN_ON_COND(!server, RETCODE_COMMON_NULL_ARG);

    server->daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, server->port, NULL, NULL, &accessHandlerCallback,
                                      server, MHD_OPTION_END);

    return (server->daemon != NULL) ? RETCODE_OK : RETCODE_COMMON_ERROR;
}

RetCode httpServerStop(HttpServer *server)
{
    RETURN_ON_COND(!server->daemon, RETCODE_COMMON_NULL_ARG);

    MHD_stop_daemon(server->daemon);
    server->daemon = NULL;

    return RETCODE_OK;
}

RetCode httpServerDestroy(HttpServer *server)
{
    RETURN_ON_COND(!server, RETCODE_COMMON_NOT_INITIALIZED);

    RetCode ret_code = RETCODE_OK;

    if(server->daemon) {
        LOG_ON_ERROR(httpServerStop(server));
    }

    free(server);

    return ret_code;
}
