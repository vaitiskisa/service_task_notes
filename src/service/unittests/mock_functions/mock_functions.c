#include "mock_functions.h"

#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int g_repo_create_called = 0;
int g_repo_get_called = 0;
int g_repo_update_called = 0;
int g_repo_delete_called = 0;
int g_repo_list_all_called = 0;
int g_repo_list_tag_called = 0;

RetCode g_repo_create_ret = RETCODE_OK;
RetCode g_repo_get_ret = RETCODE_OK;
RetCode g_repo_update_ret = RETCODE_OK;
RetCode g_repo_delete_ret = RETCODE_OK;
RetCode g_repo_list_all_ret = RETCODE_OK;
RetCode g_repo_list_tag_ret = RETCODE_OK;

Note g_repo_last_note;
Note g_repo_note_store;
NoteList g_repo_list;
char g_repo_last_id[128];
char g_repo_last_tag[128];

void resetRepoState(void)
{
    g_repo_create_called = 0;
    g_repo_get_called = 0;
    g_repo_update_called = 0;
    g_repo_delete_called = 0;
    g_repo_list_all_called = 0;
    g_repo_list_tag_called = 0;

    g_repo_create_ret = RETCODE_OK;
    g_repo_get_ret = RETCODE_OK;
    g_repo_update_ret = RETCODE_OK;
    g_repo_delete_ret = RETCODE_OK;
    g_repo_list_all_ret = RETCODE_OK;
    g_repo_list_tag_ret = RETCODE_OK;

    noteFree(&g_repo_last_note);
    noteFree(&g_repo_note_store);

    for(size_t i = 0; i < g_repo_list.count; i++) {
        noteFree(&g_repo_list.items[i]);
    }
    free(g_repo_list.items);
    g_repo_list.items = NULL;
    g_repo_list.count = 0;

    g_repo_last_id[0] = '\0';
    g_repo_last_tag[0] = '\0';
}

void fillNote(Note *note, const char *id, const char *title, const char *content)
{
    noteInit(note);
    assert_int_equal(noteSetId(note, id), RETCODE_OK);
    assert_int_equal(noteSetTitle(note, title), RETCODE_OK);
    assert_int_equal(noteSetContent(note, content), RETCODE_OK);
}

static RetCode cloneNoteList(NoteList *dst, const NoteList *src)
{
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

RetCode notesRepositoryCreateNote(NotesRepository *repo, const Note *note)
{
    (void)repo;
    g_repo_create_called++;
    noteFree(&g_repo_last_note);
    noteInit(&g_repo_last_note);
    noteClone(&g_repo_last_note, note);

    if(g_repo_create_ret != RETCODE_OK) {
        return g_repo_create_ret;
    }

    noteFree(&g_repo_note_store);
    noteInit(&g_repo_note_store);
    return noteClone(&g_repo_note_store, note);
}

RetCode notesRepositoryGetNote(NotesRepository *repo, const char *id, Note *out_note)
{
    (void)repo;
    g_repo_get_called++;
    if(id) {
        snprintf(g_repo_last_id, sizeof(g_repo_last_id), "%s", id);
    }

    if(g_repo_get_ret != RETCODE_OK) {
        return g_repo_get_ret;
    }

    return noteClone(out_note, &g_repo_note_store);
}

RetCode notesRepositoryUpdateNote(NotesRepository *repo, const Note *note)
{
    (void)repo;
    g_repo_update_called++;
    noteFree(&g_repo_last_note);
    noteInit(&g_repo_last_note);
    noteClone(&g_repo_last_note, note);

    if(g_repo_update_ret != RETCODE_OK) {
        return g_repo_update_ret;
    }

    noteFree(&g_repo_note_store);
    noteInit(&g_repo_note_store);
    return noteClone(&g_repo_note_store, note);
}

RetCode notesRepositoryDeleteNote(NotesRepository *repo, const char *id)
{
    (void)repo;
    g_repo_delete_called++;
    if(id) {
        snprintf(g_repo_last_id, sizeof(g_repo_last_id), "%s", id);
    }
    return g_repo_delete_ret;
}

RetCode notesRepositoryListAll(NotesRepository *repo, NoteList *out_list)
{
    (void)repo;
    g_repo_list_all_called++;
    if(g_repo_list_all_ret != RETCODE_OK) {
        return g_repo_list_all_ret;
    }
    return cloneNoteList(out_list, &g_repo_list);
}

RetCode notesRepositoryListByTag(NotesRepository *repo, const char *tag, NoteList *out_list)
{
    (void)repo;
    g_repo_list_tag_called++;
    if(tag) {
        snprintf(g_repo_last_tag, sizeof(g_repo_last_tag), "%s", tag);
    }
    if(g_repo_list_tag_ret != RETCODE_OK) {
        return g_repo_list_tag_ret;
    }
    return cloneNoteList(out_list, &g_repo_list);
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
