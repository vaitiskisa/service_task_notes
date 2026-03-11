/**
 * @file mock_functions.c
 * @brief Helper implementations for notes repository unit tests.
 */
#include "mock_functions.h"

#include <setjmp.h>
#include <cmocka.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void fillNote(Note *note, const char *id, const char *title, const char *content, const char *tag)
{
    noteInit(note);
    assert_int_equal(noteSetId(note, id), RETCODE_OK);
    assert_int_equal(noteSetTitle(note, title), RETCODE_OK);
    assert_int_equal(noteSetContent(note, content), RETCODE_OK);
    if(tag) {
        assert_int_equal(noteAddTag(note, tag), RETCODE_OK);
    }
}

void cleanupDir(const char *path)
{
    DIR *dir = opendir(path);
    if(!dir) {
        return;
    }

    struct dirent *entry;
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full[512];
        snprintf(full, sizeof(full), "%s/%s", path, entry->d_name);
        unlink(full);
    }

    closedir(dir);
    rmdir(path);
}
