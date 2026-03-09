#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <stddef.h>

typedef struct HttpHeader {
    const char *key;
    const char *value;
} HttpHeader;

typedef struct HttpRequest {
    const char *method; /* "GET", "POST", "PUT", "DELETE" */
    const char *url;    /* full URL path, e.g. "/notes/123?tag=work" */
    const char *path;   /* path part only, e.g. "/notes/123" */
    const char *query;  /* raw query string, e.g. "tag=work", may be NULL */

    const char *body;
    size_t body_size;

    HttpHeader *headers;
    size_t header_count;
} HttpRequest;

#endif /* HTTP_REQUEST_H */