#ifndef NOTES_SERVICE_MOCK_FUNCTIONS_H
#define NOTES_SERVICE_MOCK_FUNCTIONS_H

#include "api/notes_repository.h"

#include <stddef.h>

extern int g_repo_create_called;
extern int g_repo_get_called;
extern int g_repo_update_called;
extern int g_repo_delete_called;
extern int g_repo_list_all_called;
extern int g_repo_list_tag_called;

extern RetCode g_repo_create_ret;
extern RetCode g_repo_get_ret;
extern RetCode g_repo_update_ret;
extern RetCode g_repo_delete_ret;
extern RetCode g_repo_list_all_ret;
extern RetCode g_repo_list_tag_ret;

extern Note g_repo_last_note;
extern Note g_repo_note_store;
extern NoteList g_repo_list;
extern char g_repo_last_id[128];
extern char g_repo_last_tag[128];

void resetRepoState(void);
void fillNote(Note *note, const char *id, const char *title, const char *content);

#endif /* NOTES_SERVICE_MOCK_FUNCTIONS_H */
