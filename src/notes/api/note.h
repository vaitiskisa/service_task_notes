#ifndef MODEL_NOTE_H
#define MODEL_NOTE_H

#include "api/error_codes.h"

#include <stddef.h>
#include <jansson.h>

/*
 * Core domain structure for a note.
 */
typedef struct Note {
    char *id;
    char *title;
    char *content;

    char **tags;
    size_t tag_count;
} Note;

/*
 * Lifecycle
 */
RetCode noteInit(Note *note);
RetCode noteFree(Note *note);

/*
 * Creation helpers
 */
Note *noteCreate(const char *id, const char *title, const char *content);

/*
 * Tag manipulation
 */
RetCode noteAddTag(Note *note, const char *tag);
RetCode noteSetTags(Note *note, char **tags, size_t count);

/*
 * JSON conversion
 */
json_t *noteToJson(const Note *note);
RetCode noteFromJson(Note *note, const json_t *json);

/*
 * Validation
 */
RetCode noteValidate(const Note *note);

#endif