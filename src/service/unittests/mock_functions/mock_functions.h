#ifndef NOTES_SERVICE_MOCK_FUNCTIONS_H
#define NOTES_SERVICE_MOCK_FUNCTIONS_H

/**
 * @file mock_functions.h
 * @brief Mocks and helpers for notes service unit tests.
 */

#include "api/notes_repository.h"

#include <stddef.h>

/** Count of create calls. */
extern int g_repo_create_called;
/** Count of get calls. */
extern int g_repo_get_called;
/** Count of update calls. */
extern int g_repo_update_called;
/** Count of delete calls. */
extern int g_repo_delete_called;
/** Count of list-all calls. */
extern int g_repo_list_all_called;
/** Count of list-by-tag calls. */
extern int g_repo_list_tag_called;

/** Return code for create operation. */
extern RetCode g_repo_create_ret;
/** Return code for get operation. */
extern RetCode g_repo_get_ret;
/** Return code for update operation. */
extern RetCode g_repo_update_ret;
/** Return code for delete operation. */
extern RetCode g_repo_delete_ret;
/** Return code for list-all operation. */
extern RetCode g_repo_list_all_ret;
/** Return code for list-by-tag operation. */
extern RetCode g_repo_list_tag_ret;

/** Last note passed to create/update stubs. */
extern Note g_repo_last_note;
/** Stored note for get stubs. */
extern Note g_repo_note_store;
/** Stored list for list stubs. */
extern NoteList g_repo_list;
/** Last id captured by get/delete stubs. */
extern char g_repo_last_id[128];
/** Last tag captured by list-by-tag stub. */
extern char g_repo_last_tag[128];

/**
 * @brief Reset mock state to defaults.
 */
void resetRepoState(void);

/**
 * @brief Populate a note with id/title/content.
 *
 * @param note Note instance to populate.
 * @param id Note id (may be NULL).
 * @param title Note title.
 * @param content Note content.
 */
void fillNote(Note *note, const char *id, const char *title, const char *content);

#endif /* NOTES_SERVICE_MOCK_FUNCTIONS_H */
