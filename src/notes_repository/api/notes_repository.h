#ifndef REPOSITORY_NOTES_REPOSITORY_H
#define REPOSITORY_NOTES_REPOSITORY_H

#include "api/error_codes.h"
#include "api/note.h"

#include <stddef.h>

typedef struct NotesRepository NotesRepository;

typedef struct NoteList {
    Note *items;
    size_t count;
} NoteList;

NotesRepository *notesRepositoryCreate(const char *base_dir);
RetCode notesRepositoryDestroy(NotesRepository *repo);

RetCode notesRepositoryCreateNote(NotesRepository *repo, const Note *note);
RetCode notesRepositoryGetNote(NotesRepository *repo, const char *id, Note *out_note);
RetCode noteRepositoryUpdateNote(NotesRepository *repo, const Note *note);
RetCode notes_repository_delete_note(NotesRepository *repo, const char *id);

RetCode notesRepositoryListAll(NotesRepository *repo, NoteList *out_list);
RetCode notesRepositoryListByTag(NotesRepository *repo, const char *tag, NoteList *out_list);

RetCode noteListFree(NoteList *list);

const char *notesRepositoryResultStr(RetCode result);

#endif /* REPOSITORY_NOTES_REPOSITORY_H */