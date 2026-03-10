#include "api/notes_service.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct NotesService {
    NotesRepository *repo;
};

static int generateNoteId(char *buffer, size_t size)
{
    if(!buffer || size < 32) {
        return -1;
    }

    static unsigned long counter = 0;
    unsigned long current = ++counter;

    int written = snprintf(buffer, size, "%lu", current);
    return (written > 0 && (size_t)written < size) ? 0 : -1;
}

NotesService *notesServiceCreate(NotesRepository *repo)
{
    if(!repo) {
        return NULL;
    }

    NotesService *service = calloc(1, sizeof(*service));
    if(!service) {
        return NULL;
    }

    service->repo = repo;
    return service;
}

RetCode notesServiceDestroy(NotesService *service)
{
    RETURN_ON_COND(!service, RETCODE_COMMON_NOT_INITIALIZED);

    free(service);

    return RETCODE_OK;
}

RetCode notesServiceCreateNote(NotesService *service, const Note *input_note, Note *out_note)
{
    RETURN_ON_COND(!service, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(!input_note, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(!out_note, RETCODE_COMMON_NULL_ARG);

    RetCode ret_code = RETCODE_OK;

    RETURN_ON_COND(noteValidateForCreate(input_note) != RETCODE_OK, RETCODE_NOTES_SERVICE_VALIDATION);

    Note note_to_store;
    RETURN_ON_COND(noteInit(&note_to_store), RETCODE_COMMON_NO_MEMORY);

    TO_EXIT_ON_COND(noteClone(&note_to_store, input_note) != RETCODE_OK, RETCODE_NOTES_SERVICE_NO_MEMORY);

    char id_buffer[64];
    TO_EXIT_ON_COND(generateNoteId(id_buffer, sizeof(id_buffer)) != 0 ||
                            noteSetId(&note_to_store, id_buffer) != RETCODE_OK,
                    RETCODE_NOTES_SERVICE_NO_MEMORY);

    TO_EXIT_ON_COND(notesRepositoryCreateNote(service->repo, &note_to_store) != RETCODE_OK, RETCODE_COMMON_NO_MEMORY);

    LOG_ON_ERROR(noteFree(out_note));

    TO_EXIT_ON_COND(noteClone(out_note, &note_to_store) != RETCODE_OK, RETCODE_NOTES_SERVICE_NO_MEMORY);

    LOG_ON_ERROR(noteFree(&note_to_store));
    return RETCODE_OK;
EXIT:
    LOG_ON_ERROR(noteFree(&note_to_store));
    return RETCODE_COMMON_ERROR;
}

RetCode notesServiceGetNote(NotesService *service, const char *id, Note *out_note)
{
    RETURN_ON_COND(!service, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(!id, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(!out_note, RETCODE_COMMON_NULL_ARG);

    RetCode ret_code = RETCODE_OK;

    LOG_ON_ERROR(noteFree(out_note));

    RETURN_ON_COND(notesRepositoryGetNote(service->repo, id, out_note) != RETCODE_OK, RETCODE_COMMON_ERROR);

    return RETCODE_OK;
}

RetCode notesServiceUpdateNote(NotesService *service, const char *id, const Note *input_note, Note *out_note)
{
    RETURN_ON_COND(!service, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(!id, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(!input_note, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(!out_note, RETCODE_COMMON_NULL_ARG);

    RetCode ret_code = RETCODE_OK;

    RETURN_ON_COND(noteValidateForCreate(input_note) != RETCODE_OK, RETCODE_NOTES_SERVICE_VALIDATION);

    Note note_to_store;
    RETURN_ON_COND(noteInit(&note_to_store) != RETCODE_OK, RETCODE_COMMON_NO_MEMORY);

    TO_EXIT_ON_COND(noteClone(&note_to_store, input_note) != RETCODE_OK, RETCODE_NOTES_SERVICE_NO_MEMORY);
    TO_EXIT_ON_COND(noteSetId(&note_to_store, id) != RETCODE_OK, RETCODE_NOTES_SERVICE_NO_MEMORY);

    TO_EXIT_ON_COND(notesRepositoryUpdateNote(service->repo, &note_to_store) != RETCODE_OK, RETCODE_COMMON_NO_MEMORY);

    LOG_ON_ERROR(noteFree(out_note));

    TO_EXIT_ON_COND(noteClone(out_note, &note_to_store) != RETCODE_OK, RETCODE_NOTES_SERVICE_NO_MEMORY);

    LOG_ON_ERROR(noteFree(&note_to_store));
    return RETCODE_OK;
EXIT:
    LOG_ON_ERROR(noteFree(&note_to_store));
    return RETCODE_COMMON_ERROR;
}

RetCode notesServiceDeleteNote(NotesService *service, const char *id)
{
    RETURN_ON_COND(!service, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(!id, RETCODE_COMMON_NULL_ARG);

    RETURN_ON_COND(notesRepositoryDeleteNote(service->repo, id) != RETCODE_OK, RETCODE_COMMON_ERROR);

    return RETCODE_OK;
}

RetCode notesServiceListNotes(NotesService *service, const char *tag, NoteList *out_list)
{
    RETURN_ON_COND(!service, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(!out_list, RETCODE_COMMON_NULL_ARG);

    RetCode ret_code = RETCODE_OK;

    if(tag && *tag != '\0') {
        ret_code = notesRepositoryListByTag(service->repo, tag, out_list);
    } else {
        ret_code = notesRepositoryListAll(service->repo, out_list);
    }

    return ret_code;
}

const char *notesServiceResultStr(RetCode result)
{
    switch(result) {
    case RETCODE_OK:
        return "ok";
    case RETCODE_NOTES_SERVICE_INVALID_ARG:
        return "invalid argument";
    case RETCODE_NOTES_SERVICE_VALIDATION:
        return "validation error";
    case RETCODE_NOTES_SERVICE_NO_MEMORY:
        return "out of memory";
    case RETCODE_NOTES_SERVICE_NOT_FOUND:
        return "not found";
    case RETCODE_NOTES_SERVICE_ALREADY_EXISTS:
        return "already exists";
    case RETCODE_NOTES_SERVICE_STORAGE_ERROR:
    default:
        return "storage error";
    }
}