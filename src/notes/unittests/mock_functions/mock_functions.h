#ifndef NOTES_MOCK_FUNCTIONS_H
#define NOTES_MOCK_FUNCTIONS_H

/**
 * @file mock_functions.h
 * @brief Helper functions for notes unit tests.
 */

#include "api/note.h"

/**
 * @brief Populate a note with a fixed id/title/content and two tags.
 *
 * Tests rely on this helper to create a consistent baseline note.
 *
 * @param note Note instance to populate.
 */
void fillNote(Note *note);

#endif /* NOTES_MOCK_FUNCTIONS_H */
