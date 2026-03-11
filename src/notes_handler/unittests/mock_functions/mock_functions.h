#ifndef NOTES_HANDLER_MOCK_FUNCTIONS_H
#define NOTES_HANDLER_MOCK_FUNCTIONS_H

#include "api/notes_service.h"

#include <stddef.h>

extern RetCode g_ret_create;
extern RetCode g_ret_get;
extern RetCode g_ret_update;
extern RetCode g_ret_delete;
extern RetCode g_ret_list;

extern Note g_create_out;
extern Note g_get_out;
extern Note g_update_out;
extern Note g_last_input;
extern int g_last_input_set;
extern char g_last_id[128];
extern char g_last_tag[128];
extern NoteList g_list_out;

void resetMocks(void);
void fillNote(Note *note, const char *id, const char *title, const char *content, const char *tag);

#endif /* NOTES_HANDLER_MOCK_FUNCTIONS_H */
