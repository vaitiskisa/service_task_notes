/**
 * @file notes_repository.h
 * @brief Persistence API for storing and retrieving notes.
 */
#ifndef NOTES_REPOSITORY_H
#define NOTES_REPOSITORY_H

#include "api/error_codes.h"
#include "api/note.h"

#include <stddef.h>

/** Opaque repository handle. */
typedef struct NotesRepository NotesRepository;

/**
 * @brief Container for lists of notes.
 */
typedef struct NoteList {
    /** Array of notes. */
    Note *items;
    /** Number of notes in @ref items. */
    size_t count;
} NoteList;

/** @name Repository lifecycle */
/**@{*/
/**
 * @brief Create a repository rooted at a base directory.
 *
 * @param base_dir Filesystem path for storage.
 * @return Repository handle or NULL on error.
 */
NotesRepository *notesRepositoryCreate(const char *base_dir);
/**
 * @brief Destroy a repository handle.
 *
 * @param repo Repository to destroy.
 * @return RETCODE_OK on success.
 */
RetCode notesRepositoryDestroy(NotesRepository *repo);
/**@}*/

/** @name CRUD operations */
/**@{*/
/**
 * @brief Create a note (fails if it already exists).
 *
 * @param repo Repository handle.
 * @param note Note to create (must have id).
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode notesRepositoryCreateNote(NotesRepository *repo, const Note *note);
/**
 * @brief Load a note by id.
 *
 * @param repo Repository handle.
 * @param id Note identifier.
 * @param out_note Destination for the loaded note.
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode notesRepositoryGetNote(NotesRepository *repo, const char *id, Note *out_note);
/**
 * @brief Update a note (fails if it does not exist).
 *
 * @param repo Repository handle.
 * @param note Note to update (must have id).
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode notesRepositoryUpdateNote(NotesRepository *repo, const Note *note);
/**
 * @brief Delete a note by id.
 *
 * @param repo Repository handle.
 * @param id Note identifier.
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode notesRepositoryDeleteNote(NotesRepository *repo, const char *id);
/**@}*/

/** @name Listing */
/**@{*/
/**
 * @brief List all notes in the repository.
 *
 * @param repo Repository handle.
 * @param out_list Output list (caller must free with @ref noteListFree).
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode notesRepositoryListAll(NotesRepository *repo, NoteList *out_list);
/**
 * @brief List notes that contain a given tag.
 *
 * @param repo Repository handle.
 * @param tag Tag to filter by. If NULL/empty, lists all notes.
 * @param out_list Output list (caller must free with @ref noteListFree).
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode notesRepositoryListByTag(NotesRepository *repo, const char *tag, NoteList *out_list);
/**@}*/

/** @name Utilities */
/**@{*/
/**
 * @brief Free a NoteList and its contents.
 *
 * @param list List to free.
 */
void noteListFree(NoteList *list);
/**@}*/

#endif /* NOTES_REPOSITORY_H */
