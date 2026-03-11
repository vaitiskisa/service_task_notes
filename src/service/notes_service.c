/**
 * @file notes_service.c
 * @brief Implementation of the notes business logic layer.
 */
#include "api/notes_service.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#define ID_BUFFER_SIZE 64
#define UUID_STR_BYTES 16

struct NotesService {
    NotesRepository *repo;
};

static int generateNoteId(char *buffer, size_t size)
{
    if(!buffer || size < 37) {
        return -1;
    }

    unsigned char bytes[UUID_STR_BYTES];
    int fd = open("/dev/urandom", O_RDONLY);
    if(fd < 0) {
        return -1;
    }

    ssize_t read_len = read(fd, bytes, sizeof(bytes));
    close(fd);
    if(read_len != (ssize_t)sizeof(bytes)) {
        return -1;
    }

    /* UUIDv4: set version and variant bits */
    bytes[6] = (bytes[6] & 0x0F) | 0x40;
    bytes[8] = (bytes[8] & 0x3F) | 0x80;

    int written = snprintf(buffer, size, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                           bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7], bytes[8],
                           bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
    return (written == 36) ? 0 : -1;
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

    RETURN_ON_COND(noteValidateForCreate(input_note) != RETCODE_OK, RETCODE_NOTES_SERVICE_VALIDATION);

    RetCode ret_code = RETCODE_OK;
    Note note_to_store;
    RETURN_ON_COND(noteInit(&note_to_store), RETCODE_COMMON_NO_MEMORY);

    if(noteClone(&note_to_store, input_note) != RETCODE_OK) {
        ret_code = RETCODE_NOTES_SERVICE_NO_MEMORY;
        goto EXIT;
    }

    char id_buffer[ID_BUFFER_SIZE];
    if(generateNoteId(id_buffer, sizeof(id_buffer)) != 0 || noteSetId(&note_to_store, id_buffer) != RETCODE_OK) {
        ret_code = RETCODE_NOTES_SERVICE_NO_MEMORY;
        goto EXIT;
    }

    RetCode repo_result = notesRepositoryCreateNote(service->repo, &note_to_store);
    if(repo_result != RETCODE_OK) {
        if(repo_result == RETCODE_NOTES_REPOSITORY_ALREADY_EXISTS) {
            ret_code = RETCODE_NOTES_SERVICE_ALREADY_EXISTS;
        } else if(repo_result == RETCODE_NOTES_REPOSITORY_NO_MEMORY) {
            ret_code = RETCODE_NOTES_SERVICE_NO_MEMORY;
        } else {
            ret_code = RETCODE_NOTES_SERVICE_STORAGE_ERROR;
        }
        goto EXIT;
    }

    noteFree(out_note);

    TO_EXIT_ON_COND(noteClone(out_note, &note_to_store) != RETCODE_OK, RETCODE_NOTES_SERVICE_NO_MEMORY);

    noteFree(&note_to_store);
    return RETCODE_OK;
EXIT:
    noteFree(&note_to_store);
    return (ret_code != RETCODE_OK) ? ret_code : RETCODE_COMMON_ERROR;
}

RetCode notesServiceGetNote(NotesService *service, const char *id, Note *out_note)
{
    RETURN_ON_COND(!service, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(!id, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(!out_note, RETCODE_COMMON_NULL_ARG);

    noteFree(out_note);

    RETURN_ON_COND(notesRepositoryGetNote(service->repo, id, out_note) != RETCODE_OK, RETCODE_COMMON_ERROR);

    return RETCODE_OK;
}

RetCode notesServiceUpdateNote(NotesService *service, const char *id, const Note *input_note, Note *out_note)
{
    RETURN_ON_COND(!service, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(!id, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(!input_note, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(!out_note, RETCODE_COMMON_NULL_ARG);

    RETURN_ON_COND(noteValidateForCreate(input_note) != RETCODE_OK, RETCODE_NOTES_SERVICE_VALIDATION);

    Note note_to_store;
    RETURN_ON_COND(noteInit(&note_to_store) != RETCODE_OK, RETCODE_COMMON_NO_MEMORY);

    TO_EXIT_ON_COND(noteClone(&note_to_store, input_note) != RETCODE_OK, RETCODE_NOTES_SERVICE_NO_MEMORY);
    TO_EXIT_ON_COND(noteSetId(&note_to_store, id) != RETCODE_OK, RETCODE_NOTES_SERVICE_NO_MEMORY);

    TO_EXIT_ON_COND(notesRepositoryUpdateNote(service->repo, &note_to_store) != RETCODE_OK, RETCODE_COMMON_NO_MEMORY);

    noteFree(out_note);

    TO_EXIT_ON_COND(noteClone(out_note, &note_to_store) != RETCODE_OK, RETCODE_NOTES_SERVICE_NO_MEMORY);

    noteFree(&note_to_store);
    return RETCODE_OK;
EXIT:
    noteFree(&note_to_store);
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
