#include "api/notes_service.h"
#include "api/notes_repository.h"
#include "mock_functions.h"

#include <string.h>
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>

static void test_service_create_note_success(void **state)
{
    (void)state;
    resetRepoState();

    NotesService *service = notesServiceCreate((NotesRepository *)0x1);
    assert_non_null(service);

    Note input;
    fillNote(&input, NULL, "Title", "Content");

    Note out;
    noteInit(&out);

    assert_int_equal(notesServiceCreateNote(service, &input, &out), RETCODE_OK);
    assert_int_equal(g_repo_create_called, 1);
    assert_non_null(out.id);
    assert_string_equal(out.title, "Title");
    assert_string_equal(g_repo_last_note.title, "Title");
    assert_non_null(g_repo_last_note.id);

    noteFree(&input);
    noteFree(&out);
    notesServiceDestroy(service);
}

static void test_service_create_note_validation_fails(void **state)
{
    (void)state;
    resetRepoState();

    NotesService *service = notesServiceCreate((NotesRepository *)0x1);
    assert_non_null(service);

    Note input;
    noteInit(&input);
    noteSetTitle(&input, "Title");
    /* content missing */

    Note out;
    noteInit(&out);

    assert_int_equal(notesServiceCreateNote(service, &input, &out), RETCODE_NOTES_SERVICE_VALIDATION);
    assert_int_equal(g_repo_create_called, 0);

    noteFree(&input);
    noteFree(&out);
    notesServiceDestroy(service);
}

static void test_service_get_note_not_found(void **state)
{
    (void)state;
    resetRepoState();
    g_repo_get_ret = RETCODE_NOTES_REPOSITORY_NOT_FOUND;

    NotesService *service = notesServiceCreate((NotesRepository *)0x1);
    assert_non_null(service);

    Note out;
    noteInit(&out);

    assert_int_equal(notesServiceGetNote(service, "missing", &out), RETCODE_COMMON_ERROR);
    assert_int_equal(g_repo_get_called, 1);
    assert_string_equal(g_repo_last_id, "missing");

    noteFree(&out);
    notesServiceDestroy(service);
}

static void test_service_update_note_success(void **state)
{
    (void)state;
    resetRepoState();

    NotesService *service = notesServiceCreate((NotesRepository *)0x1);
    assert_non_null(service);

    Note input;
    fillNote(&input, NULL, "Title", "Content");

    Note out;
    noteInit(&out);

    assert_int_equal(notesServiceUpdateNote(service, "id-9", &input, &out), RETCODE_OK);
    assert_int_equal(g_repo_update_called, 1);
    assert_string_equal(g_repo_last_note.id, "id-9");
    assert_string_equal(out.id, "id-9");

    noteFree(&input);
    noteFree(&out);
    notesServiceDestroy(service);
}

static void test_service_delete_note_success(void **state)
{
    (void)state;
    resetRepoState();

    NotesService *service = notesServiceCreate((NotesRepository *)0x1);
    assert_non_null(service);

    assert_int_equal(notesServiceDeleteNote(service, "id-1"), RETCODE_OK);
    assert_int_equal(g_repo_delete_called, 1);
    assert_string_equal(g_repo_last_id, "id-1");

    notesServiceDestroy(service);
}

static void test_service_list_notes_uses_tag(void **state)
{
    (void)state;
    resetRepoState();

    g_repo_list.count = 1;
    g_repo_list.items = calloc(1, sizeof(Note));
    fillNote(&g_repo_list.items[0], "id-1", "T1", "C1");

    NotesService *service = notesServiceCreate((NotesRepository *)0x1);
    assert_non_null(service);

    NoteList out = { 0 };
    assert_int_equal(notesServiceListNotes(service, "work", &out), RETCODE_OK);
    assert_int_equal(g_repo_list_tag_called, 1);
    assert_string_equal(g_repo_last_tag, "work");

    noteListFree(&out);
    notesServiceDestroy(service);
}

static void test_service_list_notes_all_when_no_tag(void **state)
{
    (void)state;
    resetRepoState();

    g_repo_list.count = 1;
    g_repo_list.items = calloc(1, sizeof(Note));
    fillNote(&g_repo_list.items[0], "id-1", "T1", "C1");

    NotesService *service = notesServiceCreate((NotesRepository *)0x1);
    assert_non_null(service);

    NoteList out = { 0 };
    assert_int_equal(notesServiceListNotes(service, "", &out), RETCODE_OK);
    assert_int_equal(g_repo_list_all_called, 1);

    noteListFree(&out);
    notesServiceDestroy(service);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_service_create_note_success),
        cmocka_unit_test(test_service_create_note_validation_fails),
        cmocka_unit_test(test_service_get_note_not_found),
        cmocka_unit_test(test_service_update_note_success),
        cmocka_unit_test(test_service_delete_note_success),
        cmocka_unit_test(test_service_list_notes_uses_tag),
        cmocka_unit_test(test_service_list_notes_all_when_no_tag),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
