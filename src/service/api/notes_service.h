#ifndef SERVICE_NOTES_SERVICE_H
#define SERVICE_NOTES_SERVICE_H

#include "api/note.h"
#include "api/notes_repository.h"

typedef struct NotesService NotesService;

NotesService *notesServiceCreate(NotesRepository *repo);
RetCode notesServiceDestroy(NotesService *service);

RetCode notesServiceCreateNote(NotesService *service, const Note *input_note, Note *out_note);

RetCode notesServiceGetNote(NotesService *service, const char *id, Note *out_note);

RetCode notesServiceUpdateNote(NotesService *service, const char *id, const Note *input_note, Note *out_note);

RetCode notesServiceDeleteNote(NotesService *service, const char *id);

RetCode notesServiceListNotes(NotesService *service, const char *tag, NoteList *out_list);

const char *notesServiceResultStr(RetCode result);

#endif