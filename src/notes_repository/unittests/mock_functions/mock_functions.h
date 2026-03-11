#ifndef NOTES_REPOSITORY_MOCK_FUNCTIONS_H
#define NOTES_REPOSITORY_MOCK_FUNCTIONS_H

#include "api/note.h"

void fillNote(Note *note, const char *id, const char *title, const char *content, const char *tag);
void cleanupDir(const char *path);

#endif /* NOTES_REPOSITORY_MOCK_FUNCTIONS_H */
