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

RetCode noteSetId(Note *note, const char *id);
RetCode noteSetTitle(Note *note, const char *title);
RetCode noteSetContent(Note *note, const char *content);

/*
 * Tag manipulation
 */
RetCode noteClearTags(Note *note);
RetCode noteAddTag(Note *note, const char *tag);
RetCode noteSetTags(Note *note, char **tags, size_t count);

RetCode noteClone(Note *dest, const Note *src);
int noteHasTag(const Note *note, const char *tag);

/*
 * JSON conversion
 */
json_t *noteToJson(const Note *note);
RetCode noteFromJson(Note *note, const json_t *json);

/*
 * Validation
 */
RetCode noteValidateForCreate(const Note *note);
RetCode noteValidateForUpdate(const Note *note);

#endif