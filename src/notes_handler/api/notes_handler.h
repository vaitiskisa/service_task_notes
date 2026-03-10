#ifndef API_NOTES_HANDLER_H
#define API_NOTES_HANDLER_H

#include "api/request.h"
#include "api/response.h"
#include "api/notes_service.h"
#include "api/error_codes.h"

typedef struct NotesHandlerContext {
    NotesService *service;
} NotesHandlerContext;

NotesHandlerContext *createNotesHandlerContext(NotesService *service);
RetCode destroyNotesHandlerContext(NotesHandlerContext *ctx);

HttpResponse createNoteHandler(const HttpRequest *request, void *userData);
HttpResponse listNotesHandler(const HttpRequest *request, const char *tag, void *userData);
HttpResponse getNoteHandler(const HttpRequest *request, const char *noteId, void *userData);
HttpResponse updateNoteHandler(const HttpRequest *request, const char *noteId, void *userData);
HttpResponse deleteNoteHandler(const HttpRequest *request, const char *noteId, void *userData);

#endif