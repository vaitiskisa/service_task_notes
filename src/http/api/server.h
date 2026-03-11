/**
 * @file server.h
 * @brief HTTP server abstraction built on libmicrohttpd.
 */
#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "api/router.h"
#include "api/error_codes.h"

/** Opaque server handle. */
typedef struct HttpServer HttpServer;

/**
 * @brief Server configuration.
 */
typedef struct HttpServerConfig {
    /** TCP port to listen on. */
    uint16_t port;
    /** Request handler callback. */
    HttpRouteHandler handler;
    /** User data passed to the handler. */
    void *handler_user_data;
} HttpServerConfig;

/** @name Server lifecycle */
/**@{*/
/**
 * @brief Create a server instance.
 *
 * @param config Server configuration.
 * @return Server handle or NULL on error.
 */
HttpServer *httpServerCreate(const HttpServerConfig *config);
/**
 * @brief Start the HTTP server.
 *
 * @param server Server handle.
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode httpServerStart(HttpServer *server);
/**
 * @brief Stop the HTTP server.
 *
 * @param server Server handle.
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode httpServerStop(HttpServer *server);
/**
 * @brief Destroy the server instance.
 *
 * @param server Server handle.
 * @return RETCODE_OK on success.
 */
RetCode httpServerDestroy(HttpServer *server);
/**@}*/

#endif /* HTTP_SERVER_H */
