#ifndef NOTES_HANDLER_MOCK_FUNCTIONS_H
#define NOTES_HANDLER_MOCK_FUNCTIONS_H

/**
 * @file mock_functions.h
 * @brief Mocks and helpers for notes handler unit tests.
 */

#include "api/notes_service.h"

#include <stddef.h>

/** Return value for create operation. */
extern RetCode g_ret_create;
/** Return value for get operation. */
extern RetCode g_ret_get;
/** Return value for update operation. */
extern RetCode g_ret_update;
/** Return value for delete operation. */
extern RetCode g_ret_delete;
/** Return value for list operation. */
extern RetCode g_ret_list;

/** Stub output for create operation. */
extern Note g_create_out;
/** Stub output for get operation. */
extern Note g_get_out;
/** Stub output for update operation. */
extern Note g_update_out;
/** Last input note received by a stub. */
extern Note g_last_input;
/** Whether @ref g_last_input has been set. */
extern int g_last_input_set;
/** Last note id captured by a stub. */
extern char g_last_id[128];
/** Last tag captured by a list stub. */
extern char g_last_tag[128];
/** Stub output list for list operation. */
extern NoteList g_list_out;

/**
 * @brief Reset all mock state to defaults.
 */
void resetMocks(void);

/**
 * @brief Populate a note with provided fields and optional tag.
 *
 * @param note Note instance to populate.
 * @param id Note id (may be NULL).
 * @param title Note title.
 * @param content Note content.
 * @param tag Optional tag to add.
 */
void fillNote(Note *note, const char *id, const char *title, const char *content, const char *tag);

#endif /* NOTES_HANDLER_MOCK_FUNCTIONS_H */
