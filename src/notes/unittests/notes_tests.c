#include "api/note.h"
#include "api/json_utils.h"
#include "mock_functions.h"

#include <string.h>
#include <setjmp.h>
#include <cmocka.h>

static void test_note_init_and_free(void **state)
{
    (void)state;
    Note note;
    assert_int_equal(noteInit(&note), RETCODE_OK);
    assert_null(note.id);
    assert_null(note.title);
    assert_null(note.content);
    assert_null(note.tags);
    assert_int_equal(note.tag_count, 0);

    assert_int_equal(noteSetTitle(&note, "x"), RETCODE_OK);
    assert_int_equal(noteAddTag(&note, "tag"), RETCODE_OK);
    noteFree(&note);

    assert_null(note.id);
    assert_null(note.title);
    assert_null(note.content);
    assert_null(note.tags);
    assert_int_equal(note.tag_count, 0);
}

static void test_note_setters(void **state)
{
    (void)state;
    Note note;
    assert_int_equal(noteInit(&note), RETCODE_OK);

    assert_int_equal(noteSetId(&note, "abc"), RETCODE_OK);
    assert_string_equal(note.id, "abc");

    assert_int_equal(noteSetTitle(&note, "Title"), RETCODE_OK);
    assert_string_equal(note.title, "Title");

    assert_int_equal(noteSetContent(&note, "Body"), RETCODE_OK);
    assert_string_equal(note.content, "Body");

    noteFree(&note);
}

static void test_note_tags_add_clear_has(void **state)
{
    (void)state;
    Note note;
    assert_int_equal(noteInit(&note), RETCODE_OK);

    assert_int_equal(noteAddTag(&note, "a"), RETCODE_OK);
    assert_int_equal(noteAddTag(&note, "b"), RETCODE_OK);

    assert_int_equal(note.tag_count, 2);
    assert_true(noteHasTag(&note, "a"));
    assert_true(noteHasTag(&note, "b"));
    assert_false(noteHasTag(&note, "c"));

    noteClearTags(&note);
    assert_int_equal(note.tag_count, 0);
    assert_null(note.tags);

    noteFree(&note);
}

static void test_note_set_tags_replaces(void **state)
{
    (void)state;
    Note note;
    assert_int_equal(noteInit(&note), RETCODE_OK);
    assert_int_equal(noteAddTag(&note, "old"), RETCODE_OK);

    char *tags[] = { "x", "y", "z" };
    assert_int_equal(noteSetTags(&note, tags, 3), RETCODE_OK);

    assert_int_equal(note.tag_count, 3);
    assert_true(noteHasTag(&note, "x"));
    assert_true(noteHasTag(&note, "y"));
    assert_true(noteHasTag(&note, "z"));
    assert_false(noteHasTag(&note, "old"));

    noteFree(&note);
}

static void test_note_clone_deep_copy(void **state)
{
    (void)state;
    Note src;
    fillNote(&src);

    Note dst;
    assert_int_equal(noteInit(&dst), RETCODE_OK);
    assert_int_equal(noteClone(&dst, &src), RETCODE_OK);

    assert_string_equal(dst.id, src.id);
    assert_string_equal(dst.title, src.title);
    assert_string_equal(dst.content, src.content);
    assert_int_equal(dst.tag_count, src.tag_count);
    assert_true(noteHasTag(&dst, "work"));
    assert_true(noteHasTag(&dst, "home"));

    free(src.title);
    src.title = strdupSafe("Changed");
    assert_string_not_equal(dst.title, src.title);

    noteFree(&src);
    noteFree(&dst);
}

static void test_note_json_roundtrip(void **state)
{
    (void)state;
    Note note;
    fillNote(&note);

    json_t *json = noteToJson(&note);
    assert_non_null(json);

    Note parsed;
    assert_int_equal(noteInit(&parsed), RETCODE_OK);
    assert_int_equal(noteFromJson(&parsed, json), RETCODE_OK);
    json_decref(json);

    assert_string_equal(parsed.id, note.id);
    assert_string_equal(parsed.title, note.title);
    assert_string_equal(parsed.content, note.content);
    assert_int_equal(parsed.tag_count, note.tag_count);
    assert_true(noteHasTag(&parsed, "work"));
    assert_true(noteHasTag(&parsed, "home"));

    noteFree(&note);
    noteFree(&parsed);
}

static void test_note_from_json_invalid(void **state)
{
    (void)state;
    Note note;
    assert_int_equal(noteInit(&note), RETCODE_OK);

    json_t *json = json_object();
    json_object_set_new(json, "id", json_integer(123));
    assert_int_equal(noteFromJson(&note, json), RETCODE_COMMON_ERROR);

    json_decref(json);
    noteFree(&note);
}

static void test_note_validation(void **state)
{
    (void)state;
    Note note;
    assert_int_equal(noteInit(&note), RETCODE_OK);

    assert_int_equal(noteValidateForCreate(&note), RETCODE_NOTES_INVALID);
    assert_int_equal(noteValidateForUpdate(&note), RETCODE_NOTES_INVALID);

    assert_int_equal(noteSetTitle(&note, "t"), RETCODE_OK);
    assert_int_equal(noteSetContent(&note, "c"), RETCODE_OK);
    assert_int_equal(noteValidateForCreate(&note), RETCODE_OK);
    assert_int_equal(noteValidateForUpdate(&note), RETCODE_NOTES_INVALID);

    assert_int_equal(noteSetId(&note, "id"), RETCODE_OK);
    assert_int_equal(noteValidateForUpdate(&note), RETCODE_OK);

    noteFree(&note);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_note_init_and_free),      cmocka_unit_test(test_note_setters),
        cmocka_unit_test(test_note_tags_add_clear_has), cmocka_unit_test(test_note_set_tags_replaces),
        cmocka_unit_test(test_note_clone_deep_copy),    cmocka_unit_test(test_note_json_roundtrip),
        cmocka_unit_test(test_note_from_json_invalid),  cmocka_unit_test(test_note_validation),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
