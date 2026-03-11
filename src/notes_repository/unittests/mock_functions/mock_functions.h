#ifndef NOTES_REPOSITORY_MOCK_FUNCTIONS_H
#define NOTES_REPOSITORY_MOCK_FUNCTIONS_H

/**
 * @file mock_functions.h
 * @brief Helpers for notes repository unit tests.
 */

#include "api/note.h"

/**
 * @brief Populate a note with provided fields and optional tag.
 *
 * @param note Note instance to populate.
 * @param id Note id.
 * @param title Note title.
 * @param content Note content.
 * @param tag Optional tag to add.
 */
void fillNote(Note *note, const char *id, const char *title, const char *content, const char *tag);

/**
 * @brief Remove files in a directory and delete the directory.
 *
 * @param path Directory path.
 */
void cleanupDir(const char *path);

#endif /* NOTES_REPOSITORY_MOCK_FUNCTIONS_H */
