#include "mock_functions.h"

#include <setjmp.h>
#include <cmocka.h>

void fillNote(Note *note)
{
    noteInit(note);
    assert_int_equal(noteSetId(note, "id-123"), RETCODE_OK);
    assert_int_equal(noteSetTitle(note, "Title"), RETCODE_OK);
    assert_int_equal(noteSetContent(note, "Content"), RETCODE_OK);
    assert_int_equal(noteAddTag(note, "work"), RETCODE_OK);
    assert_int_equal(noteAddTag(note, "home"), RETCODE_OK);
}
