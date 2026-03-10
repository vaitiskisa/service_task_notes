#ifndef HELPER_H
#define HELPER_H

#include "api/request.h"
#include "api/response.h"

#include <jansson.h>

#define HTTP_CONTENT_TYPE_JSON "application/json"

char *strdupSafe(const char *s);

HttpResponse makeJsonStringResponse(unsigned int statusCode, const char *jsonBody);
HttpResponse makeJsonObjectResponse(unsigned int statusCode, json_t *obj);
HttpResponse makeErrorResponse(unsigned int statusCode, const char *message);

int parseJsonRequestBody(const HttpRequest *request, json_t **outJson);

#endif /* HELPER_H */