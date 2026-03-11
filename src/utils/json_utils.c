/**
 * @file json_utils.c
 * @brief Implementation of JSON and HTTP response helpers.
 */
#include "api/json_utils.h"

#include <string.h>
#include <stdlib.h>

char *strdupSafe(const char *s)
{
    if(!s) {
        return NULL;
    }

    size_t len = strlen(s);
    char *copy = malloc(len + 1);
    if(!copy) {
        return NULL;
    }

    memcpy(copy, s, len + 1);
    return copy;
}

HttpResponse makeJsonStringResponse(unsigned int statusCode, const char *jsonBody)
{
    HttpResponse response = { 0 };

    if(!jsonBody) {
        jsonBody = "{}";
    }

    response.status_code = statusCode;
    response.content_type = strdupSafe(HTTP_CONTENT_TYPE_JSON);
    response.body = strdupSafe(jsonBody);
    response.body_size = response.body ? strlen(response.body) : 0;

    if(!response.content_type || !response.body) {
        httpResponseFree(&response);
        response.status_code = HTTP_STATUS_INTERNAL_ERROR;
        response.content_type = strdupSafe(HTTP_CONTENT_TYPE_JSON);
        response.body = strdupSafe("{\"error\":\"Out of memory\"}");
        response.body_size = response.body ? strlen(response.body) : 0;
    }

    return response;
}

HttpResponse makeJsonObjectResponse(unsigned int statusCode, json_t *obj)
{
    if(!obj) {
        return makeJsonStringResponse(HTTP_STATUS_INTERNAL_ERROR, "{\"error\":\"Failed to build JSON response\"}");
    }

    char *dump = json_dumps(obj, JSON_COMPACT);
    json_decref(obj);

    if(!dump) {
        return makeJsonStringResponse(HTTP_STATUS_INTERNAL_ERROR, "{\"error\":\"Failed to serialize JSON response\"}");
    }

    HttpResponse response = { 0 };
    response.status_code = statusCode;
    response.content_type = strdupSafe(HTTP_CONTENT_TYPE_JSON);
    response.body = strdupSafe(dump);
    response.body_size = response.body ? strlen(response.body) : 0;

    free(dump);

    if(!response.content_type || !response.body) {
        httpResponseFree(&response);
        return makeJsonStringResponse(HTTP_STATUS_INTERNAL_ERROR, "{\"error\":\"Out of memory\"}");
    }

    return response;
}

HttpResponse makeErrorResponse(unsigned int statusCode, const char *message)
{
    if(!message) {
        message = "Unknown error";
    }

    json_t *obj = json_object();
    if(!obj) {
        return makeJsonStringResponse(HTTP_STATUS_INTERNAL_ERROR, "{\"error\":\"Out of memory\"}");
    }

    if(json_object_set_new(obj, "error", json_string(message)) != 0) {
        json_decref(obj);
        return makeJsonStringResponse(HTTP_STATUS_INTERNAL_ERROR, "{\"error\":\"Out of memory\"}");
    }

    return makeJsonObjectResponse(statusCode, obj);
}

int parseJsonRequestBody(const HttpRequest *request, json_t **outJson)
{
    if(!request || !outJson) {
        return -1;
    }

    *outJson = NULL;

    if(!request->body || request->body_size == 0) {
        return -1;
    }

    json_error_t error;
    json_t *json = json_loadb(request->body, request->body_size, 0, &error);
    if(!json) {
        return -1;
    }

    *outJson = json;
    return 0;
}
