#ifndef HTTP_MOCK_FUNCTIONS_H
#define HTTP_MOCK_FUNCTIONS_H

#include "api/response.h"
#include "api/request.h"

typedef struct HandlerCapture {
    char tag[128];
    char note_id[128];
    int called_create;
    int called_list;
    int called_get;
    int called_update;
    int called_delete;
} HandlerCapture;

void resetCapture(HandlerCapture *cap);
HttpResponse handleCreate(const HttpRequest *request, void *user_data);
HttpResponse handleList(const HttpRequest *request, const char *tag, void *user_data);
HttpResponse handleGet(const HttpRequest *request, const char *note_id, void *user_data);
HttpResponse handleUpdate(const HttpRequest *request, const char *note_id, void *user_data);
HttpResponse handleDelete(const HttpRequest *request, const char *note_id, void *user_data);
HttpResponse handleNoop(const HttpRequest *request, void *user_data);

#endif /* HTTP_MOCK_FUNCTIONS_H */