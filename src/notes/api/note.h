/**
 * @file note.h
 * @brief Note domain model and API for lifecycle, tags, JSON conversion, and validation.
 */
#ifndef NOTES_API_H
#define NOTES_API_H

#include "api/error_codes.h"

#include <stddef.h>
#include <jansson.h>

/**
 * @brief Core domain structure for a note.
 */
typedef struct Note {
    /** Unique identifier string. */
    char *id;
    /** Title text. */
    char *title;
    /** Content/body text. */
    char *content;

    /** Array of tag strings. */
    char **tags;
    /** Number of tags in @ref tags. */
    size_t tag_count;
} Note;

/** @name Lifecycle */
/**@{*/
/**
 * @brief Initialize a Note to an empty state.
 *
 * @param note Note instance to initialize.
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode noteInit(Note *note);
/**
 * @brief Free all resources owned by a Note and reset its fields.
 *
 * @param note Note instance to free.
 */
void noteFree(Note *note);
/**@}*/

/** @name Setters */
/**@{*/
/**
 * @brief Set the note identifier (makes an internal copy).
 *
 * @param note Note instance.
 * @param id Identifier string (may be NULL to clear).
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode noteSetId(Note *note, const char *id);
/**
 * @brief Set the note title (makes an internal copy).
 *
 * @param note Note instance.
 * @param title Title string (may be NULL to clear).
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode noteSetTitle(Note *note, const char *title);
/**
 * @brief Set the note content (makes an internal copy).
 *
 * @param note Note instance.
 * @param content Content string (may be NULL to clear).
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode noteSetContent(Note *note, const char *content);
/**@}*/

/** @name Tag manipulation */
/**@{*/
/**
 * @brief Remove all tags from a note.
 *
 * @param note Note instance.
 */
void noteClearTags(Note *note);
/**
 * @brief Add a tag to a note (makes an internal copy).
 *
 * @param note Note instance.
 * @param tag Tag string.
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode noteAddTag(Note *note, const char *tag);
/**
 * @brief Replace all tags with the provided list (makes internal copies).
 *
 * @param note Note instance.
 * @param tags Array of tag strings.
 * @param count Number of tags.
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode noteSetTags(Note *note, char **tags, size_t count);

/**
 * @brief Deep-copy a note.
 *
 * @param dest Destination note (will be overwritten).
 * @param src Source note.
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode noteClone(Note *dest, const Note *src);
/**
 * @brief Check if a note has a tag.
 *
 * @param note Note instance.
 * @param tag Tag string to search for.
 * @return 1 if found, 0 otherwise.
 */
int noteHasTag(const Note *note, const char *tag);
/**@}*/

/** @name JSON conversion */
/**@{*/
/**
 * @brief Convert a note to a JSON object.
 *
 * @param note Note instance.
 * @return New json_t object or NULL on error.
 */
json_t *noteToJson(const Note *note);
/**
 * @brief Populate a note from a JSON object.
 *
 * @param note Note instance to populate.
 * @param json JSON object.
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode noteFromJson(Note *note, const json_t *json);
/**@}*/

/** @name Validation */
/**@{*/
/**
 * @brief Validate note fields for create operations.
 *
 * @param note Note instance.
 * @return RETCODE_OK if valid, otherwise RETCODE_NOTES_INVALID or error code.
 */
RetCode noteValidateForCreate(const Note *note);
/**
 * @brief Validate note fields for update operations.
 *
 * @param note Note instance.
 * @return RETCODE_OK if valid, otherwise RETCODE_NOTES_INVALID or error code.
 */
RetCode noteValidateForUpdate(const Note *note);
/**@}*/

#endif /* NOTES_API_H */
