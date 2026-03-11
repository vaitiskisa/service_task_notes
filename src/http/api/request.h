/**
 * @file request.h
 * @brief HTTP request representation used by the server and router.
 */
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <stddef.h>

/**
 * @brief HTTP header key/value pair.
 */
typedef struct HttpHeader {
    /** Header name. */
    const char *key;
    /** Header value. */
    const char *value;
} HttpHeader;

/**
 * @brief HTTP request data passed to handlers.
 */
typedef struct HttpRequest {
    /** HTTP method, e.g. "GET", "POST", "PUT", "DELETE". */
    const char *method;
    /** Full URL path including query, e.g. "/notes/123?tag=work". */
    const char *url;
    /** Path portion only, e.g. "/notes/123". */
    const char *path;
    /** Raw query string, e.g. "tag=work" (may be NULL). */
    const char *query;

    /** Request body (may be NULL). */
    const char *body;
    /** Size of @ref body in bytes. */
    size_t body_size;

    /** Array of headers (optional). */
    HttpHeader *headers;
    /** Number of headers in @ref headers. */
    size_t header_count;
} HttpRequest;

#endif /* HTTP_REQUEST_H */
