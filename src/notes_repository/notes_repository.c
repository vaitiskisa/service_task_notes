#include "api/helper.h"
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
    struct stat st;

    RETURN_ON_COND(!path, RETCODE_COMMON_PARAM_IS_NULL);

    if(stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode) ? RETCODE_SUCCESS : RETCODE_COMMON_FAIL;
    }

    if(mkdir(path, 0755) == 0) {
        return RETCODE_SUCCESS;
    }

    return RETCODE_COMMON_FAIL;
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
    RETURN_ON_COND(!path, RETCODE_NOTES_INVALID_ARG);
    RETURN_ON_COND(!note, RETCODE_NOTES_INVALID_ARG);

    json_t *root = noteToJson(note);
    RETURN_ON_COND(!root, RETCODE_NOTES_JSON_FAIL);

    char *tmp_path = buildTempPath(path);
    if(!tmp_path) {
        json_decref(root);
        return RETCODE_NOTES_NOMEM;
    }

    if(json_dump_file(root, tmp_path, JSON_INDENT(2)) != 0) {
        free(tmp_path);
        json_decref(root);
        return RETCODE_NOTES_IO_FAIL;
    }

    if(rename(tmp_path, path) != 0) {
        unlink(tmp_path);
        free(tmp_path);
        json_decref(root);
        return RETCODE_NOTES_IO_FAIL;
    }

    free(tmp_path);
    json_decref(root);
    return RETCODE_SUCCESS;
}

static RetCode readNoteJsonFile(const char *path, Note *out_note)
{
    RETURN_ON_COND(!path, RETCODE_NOTES_INVALID_ARG);
    RETURN_ON_COND(!out_note, RETCODE_NOTES_INVALID_ARG);

    json_error_t error;
    json_t *root = json_load_file(path, 0, &error);
    RETURN_ON_COND(!root, RETCODE_NOTES_JSON_FAIL);

    noteInit(out_note);

    if(noteFromJson(out_note, root) != 0) {
        json_decref(root);
        noteFree(out_note);
        return RETCODE_NOTES_JSON_FAIL;
    }

    json_decref(root);
    return RETCODE_SUCCESS;
}

static int noteHasTag(const Note *note, const char *tag)
{
    if(!note || !tag) {
        return 0;
    }

    for(size_t i = 0; i < note->tag_count; i++) {
        if(note->tags[i] && strcmp(note->tags[i], tag) == 0) {
            return 1;
        }
    }

    return 0;
}

static RetCode noteListAppend(NoteList *list, const Note *note)
{
    RETURN_ON_COND(!list, RETCODE_NOTES_INVALID_ARG);
    RETURN_ON_COND(!note, RETCODE_NOTES_INVALID_ARG);

    Note *new_items = realloc(list->items, sizeof(Note) * (list->count + 1));
    RETURN_ON_COND(!new_items, RETCODE_NOTES_NOMEM);

    list->items = new_items;
    noteInit(&list->items[list->count]);

    /*
     * Deep copy via JSON roundtrip keeps this layer simple and avoids
     * hand-copying every field.
     */
    json_t *json = noteToJson(note);
    RETURN_ON_COND(!json, RETCODE_NOTES_JSON_FAIL);

    if(noteFromJson(&list->items[list->count], json) != 0) {
        json_decref(json);
        return RETCODE_NOTES_JSON_FAIL;
    }

    json_decref(json);
    list->count++;
    return RETCODE_SUCCESS;
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
    RETURN_ON_COND(!repo, RETCODE_COMMON_NEED_INIT);

    free(repo->base_dir);
    free(repo);

    return RETCODE_SUCCESS;
}

RetCode notesRepositoryCreateNote(NotesRepository *repo, const Note *note)
{
    RETURN_ON_COND(!repo, RETCODE_NOTES_INVALID_ARG);
    RETURN_ON_COND(!note, RETCODE_NOTES_INVALID_ARG);
    RETURN_ON_COND(!note->id, RETCODE_NOTES_INVALID_ARG);

    char *path = buildNotePath(repo, note->id);
    RETURN_ON_COND(!path, RETCODE_NOTES_NOMEM);

    if(pathExists(path)) {
        free(path);
        return RETCODE_NOTES_ALREADY_EXISTS;
    }

    RetCode result = writeNoteJsonAtomic(path, note);
    free(path);
    return result;
}

RetCode notesRepositoryGetNote(NotesRepository *repo, const char *id, Note *out_note)
{
    RETURN_ON_COND(!repo, RETCODE_NOTES_INVALID_ARG);
    RETURN_ON_COND(!id, RETCODE_NOTES_INVALID_ARG);
    RETURN_ON_COND(!out_note, RETCODE_NOTES_INVALID_ARG);

    char *path = buildNotePath(repo, id);
    RETURN_ON_COND(!path, RETCODE_NOTES_NOMEM);

    if(!pathExists(path)) {
        free(path);
        return RETCODE_NOTES_NOT_FOUND;
    }

    RetCode result = readNoteJsonFile(path, out_note);
    free(path);
    return result;
}

RetCode noteRepositoryUpdateNote(NotesRepository *repo, const Note *note)
{
    RETURN_ON_COND(!repo, RETCODE_NOTES_INVALID_ARG);
    RETURN_ON_COND(!note, RETCODE_NOTES_INVALID_ARG);
    RETURN_ON_COND(!note->id, RETCODE_NOTES_INVALID_ARG);

    char *path = buildNotePath(repo, note->id);
    RETURN_ON_COND(!path, RETCODE_NOTES_NOMEM);

    if(!pathExists(path)) {
        free(path);
        return RETCODE_NOTES_NOT_FOUND;
    }

    RetCode result = writeNoteJsonAtomic(path, note);
    free(path);
    return result;
}

RetCode notes_repository_delete_note(NotesRepository *repo, const char *id)
{
    RETURN_ON_COND(!repo, RETCODE_NOTES_INVALID_ARG);
    RETURN_ON_COND(!id, RETCODE_NOTES_INVALID_ARG);

    char *path = buildNotePath(repo, id);
    RETURN_ON_COND(!path, RETCODE_NOTES_NOMEM);

    if(!pathExists(path)) {
        free(path);
        return RETCODE_NOTES_NOT_FOUND;
    }

    if(unlink(path) != 0) {
        free(path);
        return RETCODE_NOTES_IO_FAIL;
    }

    free(path);
    return RETCODE_SUCCESS;
}

RetCode notesRepositoryListAll(NotesRepository *repo, NoteList *out_list)
{
    RETURN_ON_COND(!repo, RETCODE_NOTES_INVALID_ARG);
    RETURN_ON_COND(!out_list, RETCODE_NOTES_INVALID_ARG);

    out_list->items = NULL;
    out_list->count = 0;

    DIR *dir = opendir(repo->base_dir);
    if(!dir) {
        return RETCODE_NOTES_IO_FAIL;
    }

    struct dirent *entry;
    RetCode result = RETCODE_SUCCESS;

    while((entry = readdir(dir)) != NULL) {
        if(!hasJsonExtension(entry->d_name)) {
            continue;
        }

        char *id = extractIdFromFilename(entry->d_name);
        if(!id) {
            result = RETCODE_NOTES_NOMEM;
            break;
        }

        Note note;
        noteInit(&note);

        result = notesRepositoryGetNote(repo, id, &note);
        free(id);

        if(result != RETCODE_SUCCESS) {
            noteFree(&note);
            break;
        }

        result = noteListAppend(out_list, &note);
        noteFree(&note);

        if(result != RETCODE_SUCCESS) {
            break;
        }
    }

    closedir(dir);

    if(result != RETCODE_SUCCESS) {
        noteListFree(out_list);
    }

    return result;
}

RetCode notesRepositoryListByTag(NotesRepository *repo, const char *tag, NoteList *out_list)
{
    RETURN_ON_COND(!repo, RETCODE_NOTES_INVALID_ARG);
    RETURN_ON_COND(!out_list, RETCODE_NOTES_INVALID_ARG);

    if(!tag || *tag == '\0') {
        return notesRepositoryListAll(repo, out_list);
    }

    out_list->items = NULL;
    out_list->count = 0;

    DIR *dir = opendir(repo->base_dir);
    RETURN_ON_COND(!dir, RETCODE_NOTES_IO_FAIL);

    struct dirent *entry;
    RetCode result = RETCODE_SUCCESS;

    while((entry = readdir(dir)) != NULL) {
        if(!hasJsonExtension(entry->d_name)) {
            continue;
        }

        char *id = extractIdFromFilename(entry->d_name);
        if(!id) {
            result = RETCODE_NOTES_NOMEM;
            break;
        }

        Note note;
        noteInit(&note);

        result = notesRepositoryGetNote(repo, id, &note);
        free(id);

        if(result != RETCODE_SUCCESS) {
            noteFree(&note);
            break;
        }

        if(noteHasTag(&note, tag)) {
            result = noteListAppend(out_list, &note);
        }

        noteFree(&note);

        if(result != RETCODE_SUCCESS) {
            break;
        }
    }

    closedir(dir);

    if(result != RETCODE_SUCCESS) {
        noteListFree(out_list);
    }

    return result;
}

RetCode noteListFree(NoteList *list)
{
    RETURN_ON_COND(!list, RETCODE_COMMON_NEED_INIT);

    for(size_t i = 0; i < list->count; i++) {
        noteFree(&list->items[i]);
    }

    free(list->items);
    list->items = NULL;
    list->count = 0;

    return RETCODE_SUCCESS;
}

const char *notesRepositoryResultStr(RetCode result)
{
    switch(result) {
    case RETCODE_SUCCESS:
        return "ok";
    case RETCODE_NOTES_INVALID_ARG:
        return "invalid argument";
    case RETCODE_NOTES_NOMEM:
        return "out of memory";
    case RETCODE_NOTES_IO_FAIL:
        return "io error";
    case RETCODE_NOTES_JSON_FAIL:
        return "json error";
    case RETCODE_NOTES_NOT_FOUND:
        return "not found";
    case RETCODE_NOTES_ALREADY_EXISTS:
        return "already exists";
    default:
        return "unknown";
    }
}