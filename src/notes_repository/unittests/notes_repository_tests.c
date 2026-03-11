#include "api/notes_repository.h"
#include "api/note.h"
#include "mock_functions.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>

static void test_repository_create_get_update_delete(void **state)
{
    (void)state;
    char dir_template[] = "/tmp/notes_repo_test_XXXXXX";
    char *dir = mkdtemp(dir_template);
    assert_non_null(dir);

    NotesRepository *repo = notesRepositoryCreate(dir);
    assert_non_null(repo);

    Note note;
    fillNote(&note, "id-1", "Title", "Content", "work");

    assert_int_equal(notesRepositoryCreateNote(repo, &note), RETCODE_OK);

    Note loaded;
    noteInit(&loaded);
    assert_int_equal(notesRepositoryGetNote(repo, "id-1", &loaded), RETCODE_OK);
    assert_string_equal(loaded.title, "Title");
    assert_true(noteHasTag(&loaded, "work"));

    assert_int_equal(noteSetContent(&note, "Updated"), RETCODE_OK);
    assert_int_equal(notesRepositoryUpdateNote(repo, &note), RETCODE_OK);

    noteFree(&loaded);
    noteInit(&loaded);
    assert_int_equal(notesRepositoryGetNote(repo, "id-1", &loaded), RETCODE_OK);
    assert_string_equal(loaded.content, "Updated");

    assert_int_equal(notesRepositoryDeleteNote(repo, "id-1"), RETCODE_OK);
    assert_int_equal(notesRepositoryGetNote(repo, "id-1", &loaded), RETCODE_NOTES_REPOSITORY_NOT_FOUND);

    noteFree(&note);
    noteFree(&loaded);
    notesRepositoryDestroy(repo);
    cleanupDir(dir);
}

static void test_repository_list_all_and_by_tag(void **state)
{
    (void)state;
    char dir_template[] = "/tmp/notes_repo_test_XXXXXX";
    char *dir = mkdtemp(dir_template);
    assert_non_null(dir);

    NotesRepository *repo = notesRepositoryCreate(dir);
    assert_non_null(repo);

    Note n1;
    Note n2;
    fillNote(&n1, "id-1", "T1", "C1", "home");
    fillNote(&n2, "id-2", "T2", "C2", "work");

    assert_int_equal(notesRepositoryCreateNote(repo, &n1), RETCODE_OK);
    assert_int_equal(notesRepositoryCreateNote(repo, &n2), RETCODE_OK);

    NoteList all = { 0 };
    assert_int_equal(notesRepositoryListAll(repo, &all), RETCODE_OK);
    assert_int_equal(all.count, 2);
    noteListFree(&all);

    NoteList filtered = { 0 };
    assert_int_equal(notesRepositoryListByTag(repo, "work", &filtered), RETCODE_OK);
    assert_int_equal(filtered.count, 1);
    assert_true(noteHasTag(&filtered.items[0], "work"));
    noteListFree(&filtered);

    noteFree(&n1);
    noteFree(&n2);
    notesRepositoryDestroy(repo);
    cleanupDir(dir);
}

static void test_repository_not_found(void **state)
{
    (void)state;
    char dir_template[] = "/tmp/notes_repo_test_XXXXXX";
    char *dir = mkdtemp(dir_template);
    assert_non_null(dir);

    NotesRepository *repo = notesRepositoryCreate(dir);
    assert_non_null(repo);

    Note note;
    noteInit(&note);
    assert_int_equal(notesRepositoryGetNote(repo, "missing", &note), RETCODE_NOTES_REPOSITORY_NOT_FOUND);
    assert_int_equal(notesRepositoryDeleteNote(repo, "missing"), RETCODE_NOTES_REPOSITORY_NOT_FOUND);

    noteFree(&note);
    notesRepositoryDestroy(repo);
    cleanupDir(dir);
}

static void test_repository_read_invalid_json(void **state)
{
    (void)state;
    char dir_template[] = "/tmp/notes_repo_test_XXXXXX";
    char *dir = mkdtemp(dir_template);
    assert_non_null(dir);

    NotesRepository *repo = notesRepositoryCreate(dir);
    assert_non_null(repo);

    char path[512];
    snprintf(path, sizeof(path), "%s/%s.json", dir, "bad-json");
    FILE *f = fopen(path, "w");
    assert_non_null(f);
    fputs("{invalid json", f);
    fclose(f);

    Note out;
    noteInit(&out);
    assert_int_equal(notesRepositoryGetNote(repo, "bad-json", &out), RETCODE_NOTES_REPOSITORY_JSON_ERROR);

    noteFree(&out);
    notesRepositoryDestroy(repo);
    cleanupDir(dir);
}

static void test_repository_read_invalid_schema(void **state)
{
    (void)state;
    char dir_template[] = "/tmp/notes_repo_test_XXXXXX";
    char *dir = mkdtemp(dir_template);
    assert_non_null(dir);

    NotesRepository *repo = notesRepositoryCreate(dir);
    assert_non_null(repo);

    char path[512];
    snprintf(path, sizeof(path), "%s/%s.json", dir, "bad-schema");
    FILE *f = fopen(path, "w");
    assert_non_null(f);
    fputs("{\"id\":123,\"title\":\"t\",\"content\":\"c\"}", f);
    fclose(f);

    Note out;
    noteInit(&out);
    assert_int_equal(notesRepositoryGetNote(repo, "bad-schema", &out), RETCODE_NOTES_REPOSITORY_JSON_ERROR);

    noteFree(&out);
    notesRepositoryDestroy(repo);
    cleanupDir(dir);
}

static void test_repository_write_io_error(void **state)
{
    (void)state;
    char dir_template[] = "/tmp/notes_repo_test_XXXXXX";
    char *dir = mkdtemp(dir_template);
    assert_non_null(dir);

    NotesRepository *repo = notesRepositoryCreate(dir);
    assert_non_null(repo);

    assert_int_equal(chmod(dir, 0500), 0);

    Note note;
    fillNote(&note, "id-1", "Title", "Content", "work");
    RetCode rc = notesRepositoryCreateNote(repo, &note);
    assert_int_equal(rc, RETCODE_NOTES_REPOSITORY_IO_ERROR);

    noteFree(&note);
    assert_int_equal(chmod(dir, 0700), 0);
    notesRepositoryDestroy(repo);
    cleanupDir(dir);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_repository_create_get_update_delete),
        cmocka_unit_test(test_repository_list_all_and_by_tag),
        cmocka_unit_test(test_repository_not_found),
        cmocka_unit_test(test_repository_read_invalid_json),
        cmocka_unit_test(test_repository_read_invalid_schema),
        cmocka_unit_test(test_repository_write_io_error),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
