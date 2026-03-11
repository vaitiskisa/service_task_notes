/**
 * @file notes_service.h
 * @brief Business logic layer for notes.
 */
#ifndef NOTES_SERVICE_H
#define NOTES_SERVICE_H

#include "api/note.h"
#include "api/notes_repository.h"

/** Opaque notes service handle. */
typedef struct NotesService NotesService;

/** @name Lifecycle */
/**@{*/
/**
 * @brief Create a notes service.
 *
 * @param repo Notes repository dependency.
 * @return Service handle or NULL on error.
 */
NotesService *notesServiceCreate(NotesRepository *repo);
/**
 * @brief Destroy a notes service.
 *
 * @param service Service handle.
 * @return RETCODE_OK on success.
 */
RetCode notesServiceDestroy(NotesService *service);
/**@}*/

/** @name Operations */
/**@{*/
/**
 * @brief Create a new note.
 *
 * @param service Service handle.
 * @param input_note Input note (without id).
 * @param out_note Output note (with id).
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode notesServiceCreateNote(NotesService *service, const Note *input_note, Note *out_note);
/**
 * @brief Retrieve a note by id.
 *
 * @param service Service handle.
 * @param id Note identifier.
 * @param out_note Output note.
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode notesServiceGetNote(NotesService *service, const char *id, Note *out_note);
/**
 * @brief Update a note by id.
 *
 * @param service Service handle.
 * @param id Note identifier.
 * @param input_note Input note fields.
 * @param out_note Output note.
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode notesServiceUpdateNote(NotesService *service, const char *id, const Note *input_note, Note *out_note);
/**
 * @brief Delete a note by id.
 *
 * @param service Service handle.
 * @param id Note identifier.
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode notesServiceDeleteNote(NotesService *service, const char *id);
/**
 * @brief List notes, optionally filtered by tag.
 *
 * @param service Service handle.
 * @param tag Optional tag filter (NULL/empty for all).
 * @param out_list Output list (caller frees via noteListFree).
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode notesServiceListNotes(NotesService *service, const char *tag, NoteList *out_list);
/**@}*/

#endif /* NOTES_SERVICE_H */
