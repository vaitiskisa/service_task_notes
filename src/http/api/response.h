#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "api/error_codes.h"

#include <stddef.h>

#define HTTP_STATUS_OK                 200u
#define HTTP_STATUS_CREATED            201u
#define HTTP_STATUS_NO_CONTENT         204u
#define HTTP_STATUS_BAD_REQUEST        400u
#define HTTP_STATUS_NOT_FOUND          404u
#define HTTP_STATUS_METHOD_NOT_ALLOWED 405u
#define HTTP_STATUS_CONFLICT           409u
#define HTTP_STATUS_INTERNAL_ERROR     500u

typedef struct HttpResponse {
    unsigned int status_code;
    char *content_type; /* e.g. "application/json" */
    char *body;         /* heap allocated */
    size_t body_size;
} HttpResponse;

RetCode httpResponseFree(HttpResponse *response);

#endif /* HTTP_RESPONSE_H */