#include "mock_functions.h"

#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RetCode g_ret_create = RETCODE_OK;
RetCode g_ret_get = RETCODE_OK;
RetCode g_ret_update = RETCODE_OK;
RetCode g_ret_delete = RETCODE_OK;
RetCode g_ret_list = RETCODE_OK;

Note g_create_out;
Note g_get_out;
Note g_update_out;
Note g_last_input;
int g_last_input_set = 0;
char g_last_id[128];
char g_last_tag[128];
NoteList g_list_out;

void resetMocks(void)
{
    g_ret_create = RETCODE_OK;
    g_ret_get = RETCODE_OK;
    g_ret_update = RETCODE_OK;
    g_ret_delete = RETCODE_OK;
    g_ret_list = RETCODE_OK;

    if(g_last_input_set) {
        noteFree(&g_last_input);
        g_last_input_set = 0;
    }

    noteInit(&g_create_out);
    noteInit(&g_get_out);
    noteInit(&g_update_out);

    if(g_list_out.items) {
        for(size_t i = 0; i < g_list_out.count; i++) {
            noteFree(&g_list_out.items[i]);
        }
        free(g_list_out.items);
    }
    g_list_out.items = NULL;
    g_list_out.count = 0;

    g_last_id[0] = '\0';
    g_last_tag[0] = '\0';
}

void fillNote(Note *note, const char *id, const char *title, const char *content, const char *tag)
{
    noteInit(note);
    noteSetId(note, id);
    noteSetTitle(note, title);
    noteSetContent(note, content);
    if(tag) {
        noteAddTag(note, tag);
    }
}

static RetCode cloneNoteList(NoteList *dst, const NoteList *src)
{
    if(!dst || !src) {
        return RETCODE_COMMON_NULL_ARG;
    }

    dst->items = NULL;
    dst->count = 0;

    if(src->count == 0) {
        return RETCODE_OK;
    }

    dst->items = calloc(src->count, sizeof(Note));
    if(!dst->items) {
        return RETCODE_COMMON_NO_MEMORY;
    }

    for(size_t i = 0; i < src->count; i++) {
        noteInit(&dst->items[i]);
        if(noteClone(&dst->items[i], &src->items[i]) != RETCODE_OK) {
            for(size_t j = 0; j <= i; j++) {
                noteFree(&dst->items[j]);
            }
            free(dst->items);
            dst->items = NULL;
            dst->count = 0;
            return RETCODE_COMMON_ERROR;
        }
    }

    dst->count = src->count;
    return RETCODE_OK;
}

RetCode notesServiceCreateNote(NotesService *service, const Note *input_note, Note *out_note)
{
    (void)service;
    if(!input_note || !out_note) {
        return RETCODE_NOTES_SERVICE_INVALID_ARG;
    }

    noteInit(&g_last_input);
    noteClone(&g_last_input, input_note);
    g_last_input_set = 1;

    if(g_ret_create != RETCODE_OK) {
        return g_ret_create;
    }

    return noteClone(out_note, &g_create_out);
}

RetCode notesServiceGetNote(NotesService *service, const char *id, Note *out_note)
{
    (void)service;
    if(id) {
        snprintf(g_last_id, sizeof(g_last_id), "%s", id);
    }

    if(g_ret_get != RETCODE_OK) {
        return g_ret_get;
    }

    return noteClone(out_note, &g_get_out);
}

RetCode notesServiceUpdateNote(NotesService *service, const char *id, const Note *input_note, Note *out_note)
{
    (void)service;
    if(id) {
        snprintf(g_last_id, sizeof(g_last_id), "%s", id);
    }

    if(input_note) {
        noteInit(&g_last_input);
        noteClone(&g_last_input, input_note);
        g_last_input_set = 1;
    }

    if(g_ret_update != RETCODE_OK) {
        return g_ret_update;
    }

    return noteClone(out_note, &g_update_out);
}

RetCode notesServiceDeleteNote(NotesService *service, const char *id)
{
    (void)service;
    if(id) {
        snprintf(g_last_id, sizeof(g_last_id), "%s", id);
    }
    return g_ret_delete;
}

RetCode notesServiceListNotes(NotesService *service, const char *tag, NoteList *out_list)
{
    (void)service;
    if(tag) {
        snprintf(g_last_tag, sizeof(g_last_tag), "%s", tag);
    }

    if(g_ret_list != RETCODE_OK) {
        return g_ret_list;
    }

    return cloneNoteList(out_list, &g_list_out);
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
