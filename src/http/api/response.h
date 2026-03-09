#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "api/error_codes.h"

#include <stddef.h>

typedef struct HttpResponse {
    unsigned int status_code;
    char *content_type; /* e.g. "application/json" */
    char *body;         /* heap allocated */
    size_t body_size;
} HttpResponse;

RetCode httpResponseFree(HttpResponse *response);

#endif /* HTTP_RESPONSE_H */