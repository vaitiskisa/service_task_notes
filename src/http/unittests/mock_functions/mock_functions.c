#include "api/response.h"
#include "api/request.h"
#include "api/json_utils.h"
#include "mock_functions.h"

#include <string.h>

void resetCapture(HandlerCapture *cap)
{
    memset(cap, 0, sizeof(*cap));
}

HttpResponse handleCreate(const HttpRequest *request, void *user_data)
{
    (void)request;
    HandlerCapture *cap = (HandlerCapture *)user_data;
    cap->called_create = 1;
    return makeJsonStringResponse(HTTP_STATUS_OK, "{}");
}

HttpResponse handleList(const HttpRequest *request, const char *tag, void *user_data)
{
    (void)request;
    HandlerCapture *cap = (HandlerCapture *)user_data;
    cap->called_list = 1;
    if(tag) {
        snprintf(cap->tag, sizeof(cap->tag), "%s", tag);
    }
    return makeJsonStringResponse(HTTP_STATUS_OK, "{}");
}

HttpResponse handleGet(const HttpRequest *request, const char *note_id, void *user_data)
{
    (void)request;
    HandlerCapture *cap = (HandlerCapture *)user_data;
    cap->called_get = 1;
    if(note_id) {
        snprintf(cap->note_id, sizeof(cap->note_id), "%s", note_id);
    }
    return makeJsonStringResponse(HTTP_STATUS_OK, "{}");
}

HttpResponse handleUpdate(const HttpRequest *request, const char *note_id, void *user_data)
{
    (void)request;
    HandlerCapture *cap = (HandlerCapture *)user_data;
    cap->called_update = 1;
    if(note_id) {
        snprintf(cap->note_id, sizeof(cap->note_id), "%s", note_id);
    }
    return makeJsonStringResponse(HTTP_STATUS_OK, "{}");
}

HttpResponse handleDelete(const HttpRequest *request, const char *note_id, void *user_data)
{
    (void)request;
    HandlerCapture *cap = (HandlerCapture *)user_data;
    cap->called_delete = 1;
    if(note_id) {
        snprintf(cap->note_id, sizeof(cap->note_id), "%s", note_id);
    }
    return makeJsonStringResponse(HTTP_STATUS_OK, "{}");
}

HttpResponse handleNoop(const HttpRequest *request, void *user_data)
{
    (void)request;
    (void)user_data;
    return makeJsonStringResponse(HTTP_STATUS_OK, "{}");
}
