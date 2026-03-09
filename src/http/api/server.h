#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "api/router.h"
#include "api/error_codes.h"

typedef struct HttpServer HttpServer;

typedef struct HttpServerConfig {
    uint16_t port;
    HttpRouteHandler handler;
    void *handler_user_data;
} HttpServerConfig;

HttpServer *httpServerCreate(const HttpServerConfig *config);
RetCode httpServerStart(HttpServer *server);
RetCode httpServerStop(HttpServer *server);
RetCode httpServerDestroy(HttpServer *server);

#endif /* HTTP_SERVER_H */