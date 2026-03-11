/**
 * @file notes_repository.c
 * @brief File-based persistence for notes.
 */
#include "api/json_utils.h"
#include "api/notes_repository.h"

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <jansson.h>

struct NotesRepository {
    char *base_dir;
};

static inline int pathExists(const char *path)
{
    struct stat st;
    return (path && stat(path, &st) == 0);
}

static RetCode ensureDirectoryExists(const char *path)
{
    RETURN_ON_COND(!path, RETCODE_COMMON_NULL_ARG);

    if(*path == '\0') {
        return RETCODE_COMMON_ERROR;
    }

    char tmp[PATH_MAX];
    size_t len = strlen(path);
    if(len >= sizeof(tmp)) {
        return RETCODE_COMMON_ERROR;
    }

    memcpy(tmp, path, len + 1);

    if(len > 1 && tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }

    for(char *p = tmp + 1; *p; ++p) {
        if(*p != '/') {
            continue;
        }

        *p = '\0';
        if(mkdir(tmp, 0755) != 0 && errno != EEXIST) {
            *p = '/';
            return RETCODE_COMMON_ERROR;
        }
        *p = '/';
    }

    if(mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        return RETCODE_COMMON_ERROR;
    }

    return RETCODE_OK;
}

static char *buildNotePath(const NotesRepository *repo, const char *id)
{
    if(!repo || !repo->base_dir || !id) {
        return NULL;
    }

    size_t len = strlen(repo->base_dir) + 1 + strlen(id) + strlen(".json") + 1;
    char *path = malloc(len);
    if(!path) {
        return NULL;
    }

    snprintf(path, len, "%s/%s.json", repo->base_dir, id);
    return path;
}

static char *buildTempPath(const char *final_path)
{
    if(!final_path) {
        return NULL;
    }

    size_t len = strlen(final_path) + strlen(".tmp") + 1;
    char *tmp_path = malloc(len);
    if(!tmp_path) {
        return NULL;
    }

    snprintf(tmp_path, len, "%s.tmp", final_path);
    return tmp_path;
}

static int hasJsonExtension(const char *name)
{
    if(!name) {
        return 0;
    }

    size_t len = strlen(name);
    return (len > 5 && strcmp(name + len - 5, ".json") == 0);
}

static char *extractIdFromFilename(const char *filename)
{
    if(!hasJsonExtension(filename)) {
        return NULL;
    }

    size_t len = strlen(filename) - 5;
    char *id = malloc(len + 1);
    if(!id) {
        return NULL;
    }

    memcpy(id, filename, len);
    id[len] = '\0';
    return id;
}

static RetCode writeNoteJsonAtomic(const char *path, const Note *note)
{
    RETURN_ON_COND(!path, RETCODE_NOTES_REPOSITORY_INVALID_ARG);
    RETURN_ON_COND(!note, RETCODE_NOTES_REPOSITORY_INVALID_ARG);

    json_t *root = noteToJson(note);
    RETURN_ON_COND(!root, RETCODE_NOTES_REPOSITORY_JSON_ERROR);

    char *tmp_path = buildTempPath(path);
    if(!tmp_path) {
        json_decref(root);
        return RETCODE_NOTES_REPOSITORY_NO_MEMORY;
    }

    if(json_dump_file(root, tmp_path, JSON_INDENT(2)) != 0) {
        free(tmp_path);
        json_decref(root);
        return RETCODE_NOTES_REPOSITORY_IO_ERROR;
    }

    if(rename(tmp_path, path) != 0) {
        unlink(tmp_path);
        free(tmp_path);
        json_decref(root);
        return RETCODE_NOTES_REPOSITORY_IO_ERROR;
    }

    free(tmp_path);
    json_decref(root);
    return RETCODE_OK;
}

static RetCode readNoteJsonFile(const char *path, Note *out_note)
{
    RETURN_ON_COND(!path, RETCODE_NOTES_REPOSITORY_INVALID_ARG);
    RETURN_ON_COND(!out_note, RETCODE_NOTES_REPOSITORY_INVALID_ARG);

    json_error_t error;
    json_t *root = json_load_file(path, 0, &error);
    RETURN_ON_COND(!root, RETCODE_NOTES_REPOSITORY_JSON_ERROR);

    RETURN_ON_COND(noteInit(out_note) != RETCODE_OK, RETCODE_COMMON_NO_MEMORY);

    if(noteFromJson(out_note, root) != 0) {
        json_decref(root);
        noteFree(out_note);
        return RETCODE_NOTES_REPOSITORY_JSON_ERROR;
    }

    json_decref(root);
    return RETCODE_OK;
}

static RetCode noteListAppend(NoteList *list, const Note *note)
{
    RETURN_ON_COND(!list, RETCODE_NOTES_REPOSITORY_INVALID_ARG);
    RETURN_ON_COND(!note, RETCODE_NOTES_REPOSITORY_INVALID_ARG);

    Note *new_items = realloc(list->items, sizeof(Note) * (list->count + 1));
    RETURN_ON_COND(!new_items, RETCODE_NOTES_REPOSITORY_NO_MEMORY);

    list->items = new_items;
    RETURN_ON_COND(noteInit(&list->items[list->count]) != RETCODE_OK, RETCODE_COMMON_NO_MEMORY);

    RETURN_ON_COND(noteClone(&list->items[list->count], note) != RETCODE_OK, RETCODE_NOTES_REPOSITORY_NO_MEMORY);

    list->count++;
    return RETCODE_OK;
}

NotesRepository *notesRepositoryCreate(const char *base_dir)
{
    if(!base_dir || *base_dir == '\0') {
        return NULL;
    }

    if(ensureDirectoryExists(base_dir) != 0) {
        return NULL;
    }

    NotesRepository *repo = calloc(1, sizeof(*repo));
    if(!repo) {
        return NULL;
    }

    repo->base_dir = strdupSafe(base_dir);
    if(!repo->base_dir) {
        free(repo);
        return NULL;
    }

    return repo;
}

RetCode notesRepositoryDestroy(NotesRepository *repo)
{
    RETURN_ON_COND(!repo, RETCODE_COMMON_NOT_INITIALIZED);

    free(repo->base_dir);
    free(repo);

    return RETCODE_OK;
}

RetCode notesRepositoryCreateNote(NotesRepository *repo, const Note *note)
{
    RETURN_ON_COND(!repo, RETCODE_NOTES_REPOSITORY_INVALID_ARG);
    RETURN_ON_COND(!note, RETCODE_NOTES_REPOSITORY_INVALID_ARG);
    RETURN_ON_COND(!note->id, RETCODE_NOTES_REPOSITORY_INVALID_ARG);

    char *path = buildNotePath(repo, note->id);
    RETURN_ON_COND(!path, RETCODE_NOTES_REPOSITORY_NO_MEMORY);

    if(pathExists(path)) {
        free(path);
        return RETCODE_NOTES_REPOSITORY_ALREADY_EXISTS;
    }

    RetCode result = writeNoteJsonAtomic(path, note);
    free(path);
    return result;
}

RetCode notesRepositoryGetNote(NotesRepository *repo, const char *id, Note *out_note)
{
    RETURN_ON_COND(!repo, RETCODE_NOTES_REPOSITORY_INVALID_ARG);
    RETURN_ON_COND(!id, RETCODE_NOTES_REPOSITORY_INVALID_ARG);
    RETURN_ON_COND(!out_note, RETCODE_NOTES_REPOSITORY_INVALID_ARG);

    char *path = buildNotePath(repo, id);
    RETURN_ON_COND(!path, RETCODE_NOTES_REPOSITORY_NO_MEMORY);

    if(!pathExists(path)) {
        free(path);
        return RETCODE_NOTES_REPOSITORY_NOT_FOUND;
    }

    RetCode result = readNoteJsonFile(path, out_note);
    free(path);
    return result;
}

RetCode notesRepositoryUpdateNote(NotesRepository *repo, const Note *note)
{
    RETURN_ON_COND(!repo, RETCODE_NOTES_REPOSITORY_INVALID_ARG);
    RETURN_ON_COND(!note, RETCODE_NOTES_REPOSITORY_INVALID_ARG);
    RETURN_ON_COND(!note->id, RETCODE_NOTES_REPOSITORY_INVALID_ARG);

    char *path = buildNotePath(repo, note->id);
    RETURN_ON_COND(!path, RETCODE_NOTES_REPOSITORY_NO_MEMORY);

    if(!pathExists(path)) {
        free(path);
        return RETCODE_NOTES_REPOSITORY_NOT_FOUND;
    }

    RetCode result = writeNoteJsonAtomic(path, note);
    free(path);
    return result;
}

RetCode notesRepositoryDeleteNote(NotesRepository *repo, const char *id)
{
    RETURN_ON_COND(!repo, RETCODE_NOTES_REPOSITORY_INVALID_ARG);
    RETURN_ON_COND(!id, RETCODE_NOTES_REPOSITORY_INVALID_ARG);

    char *path = buildNotePath(repo, id);
    RETURN_ON_COND(!path, RETCODE_NOTES_REPOSITORY_NO_MEMORY);

    if(!pathExists(path)) {
        free(path);
        return RETCODE_NOTES_REPOSITORY_NOT_FOUND;
    }

    if(unlink(path) != 0) {
        free(path);
        return RETCODE_NOTES_REPOSITORY_IO_ERROR;
    }

    free(path);
    return RETCODE_OK;
}

RetCode notesRepositoryListAll(NotesRepository *repo, NoteList *out_list)
{
    RETURN_ON_COND(!repo, RETCODE_NOTES_REPOSITORY_INVALID_ARG);
    RETURN_ON_COND(!out_list, RETCODE_NOTES_REPOSITORY_INVALID_ARG);

    out_list->items = NULL;
    out_list->count = 0;

    DIR *dir = opendir(repo->base_dir);
    if(!dir) {
        return RETCODE_NOTES_REPOSITORY_IO_ERROR;
    }

    struct dirent *entry;
    RetCode result = RETCODE_OK;

    while((entry = readdir(dir)) != NULL) {
        if(!hasJsonExtension(entry->d_name)) {
            continue;
        }

        char *id = extractIdFromFilename(entry->d_name);
        if(!id) {
            result = RETCODE_NOTES_REPOSITORY_NO_MEMORY;
            break;
        }

        Note note;
        RETURN_ON_COND(noteInit(&note) != RETCODE_OK, RETCODE_COMMON_NO_MEMORY);

        result = notesRepositoryGetNote(repo, id, &note);
        free(id);

        if(result != RETCODE_OK) {
            noteFree(&note);
            break;
        }

        result = noteListAppend(out_list, &note);
        noteFree(&note);

        if(result != RETCODE_OK) {
            break;
        }
    }

    closedir(dir);

    if(result != RETCODE_OK) {
        noteListFree(out_list);
    }

    return result;
}

RetCode notesRepositoryListByTag(NotesRepository *repo, const char *tag, NoteList *out_list)
{
    RETURN_ON_COND(!repo, RETCODE_NOTES_REPOSITORY_INVALID_ARG);
    RETURN_ON_COND(!out_list, RETCODE_NOTES_REPOSITORY_INVALID_ARG);

    if(!tag || *tag == '\0') {
        return notesRepositoryListAll(repo, out_list);
    }

    out_list->items = NULL;
    out_list->count = 0;

    DIR *dir = opendir(repo->base_dir);
    RETURN_ON_COND(!dir, RETCODE_NOTES_REPOSITORY_IO_ERROR);

    struct dirent *entry;
    RetCode result = RETCODE_OK;

    while((entry = readdir(dir)) != NULL) {
        if(!hasJsonExtension(entry->d_name)) {
            continue;
        }

        char *id = extractIdFromFilename(entry->d_name);
        if(!id) {
            result = RETCODE_NOTES_REPOSITORY_NO_MEMORY;
            break;
        }

        Note note;
        RETURN_ON_COND(noteInit(&note) != RETCODE_OK, RETCODE_COMMON_NO_MEMORY);

        result = notesRepositoryGetNote(repo, id, &note);
        free(id);

        if(result != RETCODE_OK) {
            noteFree(&note);
            break;
        }

        if(noteHasTag(&note, tag)) {
            result = noteListAppend(out_list, &note);
        }

        noteFree(&note);

        if(result != RETCODE_OK) {
            break;
        }
    }

    closedir(dir);

    if(result != RETCODE_OK) {
        noteListFree(out_list);
    }

    return result;
}

void noteListFree(NoteList *list)
{
    if(!list) {
        return;
    }

    for(size_t i = 0; i < list->count; i++) {
        noteFree(&list->items[i]);
    }

    free(list->items);
    list->items = NULL;
    list->count = 0;
}
