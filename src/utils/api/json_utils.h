/**
 * @file json_utils.h
 * @brief JSON and HTTP response helpers.
 */
#ifndef UTILS_JSON_UTILS_H
#define UTILS_JSON_UTILS_H

#include "api/request.h"
#include "api/response.h"

#include <jansson.h>

/** JSON Content-Type header value. */
#define HTTP_CONTENT_TYPE_JSON "application/json"

/**
 * @brief Duplicate a string safely.
 *
 * @param s Source string (may be NULL).
 * @return Newly allocated copy or NULL on error/NULL input.
 */
char *strdupSafe(const char *s);

/**
 * @brief Build an HTTP response from a JSON string.
 *
 * @param statusCode HTTP status code.
 * @param jsonBody JSON body string (NULL yields "{}").
 * @return HTTP response with JSON content type.
 */
HttpResponse makeJsonStringResponse(unsigned int statusCode, const char *jsonBody);
/**
 * @brief Build an HTTP response from a JSON object.
 *
 * @param statusCode HTTP status code.
 * @param obj JSON object (ownership transferred).
 * @return HTTP response with serialized JSON body.
 */
HttpResponse makeJsonObjectResponse(unsigned int statusCode, json_t *obj);
/**
 * @brief Build a standardized JSON error response.
 *
 * @param statusCode HTTP status code.
 * @param message Error message (may be NULL).
 * @return HTTP response with JSON error payload.
 */
HttpResponse makeErrorResponse(unsigned int statusCode, const char *message);

/**
 * @brief Parse JSON from an HTTP request body.
 *
 * @param request HTTP request.
 * @param outJson Output JSON object (caller must decref).
 * @return 0 on success, otherwise an error code.
 */
int parseJsonRequestBody(const HttpRequest *request, json_t **outJson);

#endif /* UTILS_JSON_UTILS_H */
