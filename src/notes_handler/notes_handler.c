#include "api/notes_handler.h"
#include "api/json_utils.h"

#include <jansson.h>
#include <stdlib.h>
#include <string.h>

static HttpResponse makeNoteResponse(unsigned int status, const Note *note)
{
    json_t *json = noteToJson(note);
    if(!json) {
        return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "Failed to serialize note");
    }

    return makeJsonObjectResponse(status, json);
}

static HttpResponse makeNoteListResponse(const NoteList *list)
{
    if(!list) {
        return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "Invalid note list");
    }

    json_t *array = json_array();
    if(!array) {
        return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "Out of memory");
    }

    for(size_t i = 0; i < list->count; i++) {
        json_t *obj = noteToJson(&list->items[i]);
        if(!obj) {
            json_decref(array);
            return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "Failed to serialize note list");
        }

        if(json_array_append_new(array, obj) != 0) {
            json_decref(obj);
            json_decref(array);
            return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "Failed to serialize note list");
        }
    }

    return makeJsonObjectResponse(HTTP_STATUS_OK, array);
}

static RetCode parseNoteFromRequest(const HttpRequest *request, Note *outNote)
{
    RETURN_ON_COND(!request, RETCODE_NOTES_SERVICE_INVALID_ARG);
    RETURN_ON_COND(!outNote, RETCODE_NOTES_SERVICE_INVALID_ARG);

    json_t *json = NULL;
    RETURN_ON_COND(parseJsonRequestBody(request, &json) != RETCODE_OK, RETCODE_NOTES_SERVICE_VALIDATION);

    RETURN_ON_COND(noteFromJson(outNote, json) != RETCODE_OK, RETCODE_NOTES_SERVICE_VALIDATION);
    json_decref(json);

    return RETCODE_OK;
}

static HttpResponse mapServiceError(RetCode result)
{
    switch(result) {
    case RETCODE_NOTES_SERVICE_NOT_FOUND:
        return makeErrorResponse(HTTP_STATUS_NOT_FOUND, "Note not found");

    case RETCODE_NOTES_SERVICE_ALREADY_EXISTS:
        return makeErrorResponse(HTTP_STATUS_CONFLICT, "Note already exists");

    case RETCODE_NOTES_SERVICE_VALIDATION:
        return makeErrorResponse(HTTP_STATUS_BAD_REQUEST, "Invalid note data");

    case RETCODE_NOTES_SERVICE_NO_MEMORY:
        return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "Out of memory");

    case RETCODE_NOTES_SERVICE_STORAGE_ERROR:
        return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "Storage error");

    default:
        return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "Internal error");
    }
}

NotesHandlerContext *createNotesHandlerContext(NotesService *service)
{
    if(!service) {
        return NULL;
    }

    NotesHandlerContext *ctx = calloc(1, sizeof(*ctx));
    if(!ctx) {
        return NULL;
    }

    ctx->service = service;
    return ctx;
}

RetCode destroyNotesHandlerContext(NotesHandlerContext *ctx)
{
    free(ctx);

    return RETCODE_OK;
}

HttpResponse createNoteHandler(const HttpRequest *request, void *userData)
{
    NotesHandlerContext *ctx = (NotesHandlerContext *)userData;
    if(!ctx || !ctx->service) {
        return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "Handler context missing");
    }

    RetCode ret_code = RETCODE_OK;

    Note input;
    Note created;
    TO_EXIT_ON_COND(noteInit(&input) != RETCODE_OK, RETCODE_COMMON_NO_MEMORY);
    TO_EXIT_ON_COND(noteInit(&created) != RETCODE_OK, RETCODE_COMMON_NO_MEMORY);

    ret_code = parseNoteFromRequest(request, &input);
    TO_EXIT_ON_COND(ret_code != RETCODE_OK, RETCODE_NOTES_SERVICE_VALIDATION);

    ret_code = notesServiceCreateNote(ctx->service, &input, &created);
    TO_EXIT_ON_COND(ret_code != RETCODE_OK, RETCODE_NOTES_SERVICE_VALIDATION);

    HttpResponse response = makeNoteResponse(HTTP_STATUS_CREATED, &created);

    LOG_ON_ERROR(noteFree(&input));
    LOG_ON_ERROR(noteFree(&created));
    return response;
EXIT:
    LOG_ON_ERROR(noteFree(&input));
    LOG_ON_ERROR(noteFree(&created));
    return mapServiceError(ret_code);
}

HttpResponse listNotesHandler(const HttpRequest *request, const char *tag, void *userData)
{
    (void)request;

    NotesHandlerContext *ctx = (NotesHandlerContext *)userData;
    if(!ctx || !ctx->service) {
        return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "Handler context missing");
    }

    NoteList list = { 0 };

    RetCode ret_code = notesServiceListNotes(ctx->service, tag, &list);
    TO_EXIT_ON_COND(ret_code != RETCODE_OK, RETCODE_COMMON_ERROR);

    HttpResponse resp = makeNoteListResponse(&list);
    LOG_ON_ERROR(noteListFree(&list));

    return resp;
EXIT:
    return mapServiceError(ret_code);
}

HttpResponse getNoteHandler(const HttpRequest *request, const char *noteId, void *userData)
{
    (void)request;

    NotesHandlerContext *ctx = (NotesHandlerContext *)userData;
    if(!ctx || !ctx->service) {
        return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "Handler context missing");
    }

    Note note;
    TO_EXIT_ON_COND(noteInit(&note) != RETCODE_OK, RETCODE_COMMON_NO_MEMORY);

    RetCode ret_code = notesServiceGetNote(ctx->service, noteId, &note);
    TO_EXIT_ON_COND(ret_code != RETCODE_OK, RETCODE_COMMON_ERROR);

    HttpResponse resp = makeNoteResponse(HTTP_STATUS_OK, &note);
    LOG_ON_ERROR(noteFree(&note));

    return resp;
EXIT:
    LOG_ON_ERROR(noteFree(&note));
    return mapServiceError(ret_code);
}

HttpResponse updateNoteHandler(const HttpRequest *request, const char *noteId, void *userData)
{
    NotesHandlerContext *ctx = (NotesHandlerContext *)userData;
    if(!ctx || !ctx->service) {
        return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "Handler context missing");
    }

    Note input;
    Note updated;
    TO_EXIT_ON_COND(noteInit(&input) != RETCODE_OK, RETCODE_COMMON_NO_MEMORY);
    TO_EXIT_ON_COND(noteInit(&updated) != RETCODE_OK, RETCODE_COMMON_NO_MEMORY);

    RetCode ret_code = parseNoteFromRequest(request, &input);
    TO_EXIT_ON_COND(ret_code != RETCODE_OK, RETCODE_NOTES_SERVICE_VALIDATION);

    ret_code = notesServiceUpdateNote(ctx->service, noteId, &input, &updated);
    TO_EXIT_ON_COND(ret_code != RETCODE_OK, RETCODE_NOTES_SERVICE_VALIDATION);

    HttpResponse response = makeNoteResponse(HTTP_STATUS_OK, &updated);
    LOG_ON_ERROR(noteFree(&input));
    LOG_ON_ERROR(noteFree(&updated));

    return response;
EXIT:
    LOG_ON_ERROR(noteFree(&input));
    LOG_ON_ERROR(noteFree(&updated));
    return mapServiceError(ret_code);
}

HttpResponse deleteNoteHandler(const HttpRequest *request, const char *noteId, void *userData)
{
    (void)request;

    NotesHandlerContext *ctx = (NotesHandlerContext *)userData;
    if(!ctx || !ctx->service) {
        return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "Handler context missing");
    }

    RetCode ret_code = notesServiceDeleteNote(ctx->service, noteId);
    TO_EXIT_ON_COND(ret_code != RETCODE_OK, RETCODE_NOTES_SERVICE_VALIDATION);

    json_t *obj = json_object();
    if(!obj) {
        return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "Out of memory");
    }

    if(json_object_set_new(obj, "status", json_string("deleted")) != 0) {
        json_decref(obj);
        return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "Out of memory");
    }

    return makeJsonObjectResponse(HTTP_STATUS_OK, obj);
EXIT:
    return mapServiceError(ret_code);
}
