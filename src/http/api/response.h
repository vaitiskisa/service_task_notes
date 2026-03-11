/**
 * @file response.h
 * @brief HTTP response representation.
 */
#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "api/error_codes.h"

#include <stddef.h>

/** @name Common HTTP status codes */
/**@{*/
#define HTTP_STATUS_OK                 200u
#define HTTP_STATUS_CREATED            201u
#define HTTP_STATUS_NO_CONTENT         204u
#define HTTP_STATUS_BAD_REQUEST        400u
#define HTTP_STATUS_NOT_FOUND          404u
#define HTTP_STATUS_METHOD_NOT_ALLOWED 405u
#define HTTP_STATUS_CONFLICT           409u
#define HTTP_STATUS_INTERNAL_ERROR     500u
/**@}*/

/**
 * @brief HTTP response data.
 */
typedef struct HttpResponse {
    /** HTTP status code. */
    unsigned int status_code;
    /** Content-Type header value (heap allocated). */
    char *content_type;
    /** Response body (heap allocated). */
    char *body;
    /** Size of @ref body in bytes. */
    size_t body_size;
} HttpResponse;

/**
 * @brief Free response resources and reset fields.
 *
 * @param response Response to free.
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode httpResponseFree(HttpResponse *response);

#endif /* HTTP_RESPONSE_H */
