#include "api/note.h"
#include "api/helper.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

RetCode noteInit(Note *note)
{
    RETURN_ON_COND(!note, RETCODE_COMMON_PARAM_IS_NULL);

    note->id = NULL;
    note->title = NULL;
    note->content = NULL;
    note->tags = NULL;
    note->tag_count = 0;

    return RETCODE_SUCCESS;
}

RetCode noteFree(Note *note)
{
    RETURN_ON_COND(!note, RETCODE_COMMON_PARAM_IS_NULL);

    free(note->id);
    free(note->title);
    free(note->content);

    for(size_t i = 0; i < note->tag_count; i++) {
        free(note->tags[i]);
    }

    free(note->tags);

    noteInit(note);

    return RETCODE_SUCCESS;
}

Note *noteCreate(const char *id, const char *title, const char *content)
{
    Note *note = calloc(1, sizeof(Note));
    if(!note) {
        return NULL;
    }

    noteInit(note);

    note->id = strdupSafe(id);
    note->title = strdupSafe(title);
    note->content = strdupSafe(content);

    if(!note->id || !note->title) {
        noteFree(note);
        free(note);
        return NULL;
    }

    return note;
}

RetCode noteAddTag(Note *note, const char *tag)
{
    RETURN_ON_COND(!note, RETCODE_COMMON_PARAM_IS_NULL);
    RETURN_ON_COND(!tag, RETCODE_COMMON_PARAM_IS_NULL);

    char **new_tags = realloc(note->tags, sizeof(char *) * (note->tag_count + 1));
    RETURN_ON_COND(!new_tags, RETCODE_COMMON_ALLOC_FAIL);

    note->tags = new_tags;

    note->tags[note->tag_count] = strdupSafe(tag);
    RETURN_ON_COND(!note->tags[note->tag_count], RETCODE_COMMON_STR_DUP_FAIL);

    note->tag_count++;

    return RETCODE_SUCCESS;
}

RetCode noteSetTags(Note *note, char **tags, size_t count)
{
    RETURN_ON_COND(!note, RETCODE_COMMON_PARAM_IS_NULL);
    RETURN_ON_COND(!tags, RETCODE_COMMON_PARAM_IS_NULL);
    RETURN_ON_COND(count == 0, RETCODE_COMMON_PARAM_IS_INVALID);

    for(size_t i = 0; i < note->tag_count; i++) {
        free(note->tags[i]);
    }

    free(note->tags);

    note->tags = NULL;
    note->tag_count = 0;

    note->tags = calloc(count, sizeof(char *));
    RETURN_ON_COND(!note->tags, RETCODE_COMMON_ALLOC_FAIL);

    for(size_t i = 0; i < count; i++) {
        note->tags[i] = strdupSafe(tags[i]);
        RETURN_ON_COND(!note->tags[i], RETCODE_COMMON_STR_DUP_FAIL);
    }

    note->tag_count = count;

    return RETCODE_SUCCESS;
}

json_t *noteToJson(const Note *note)
{
    if(!note) {
        return NULL;
    }

    json_t *root = json_object();

    json_object_set_new(root, "id", note->id ? json_string(note->id) : json_null());

    json_object_set_new(root, "title", note->title ? json_string(note->title) : json_null());

    json_object_set_new(root, "content", note->content ? json_string(note->content) : json_null());

    json_t *tags = json_array();

    for(size_t i = 0; i < note->tag_count; i++) {
        json_array_append_new(tags, json_string(note->tags[i]));
    }

    json_object_set_new(root, "tags", tags);

    return root;
}

RetCode noteFromJson(Note *note, const json_t *json)
{
    RETURN_ON_COND(!note, RETCODE_COMMON_PARAM_IS_NULL);
    RETURN_ON_COND(!json, RETCODE_COMMON_PARAM_IS_NULL);
    RETURN_ON_COND(!json_is_object(json), RETCODE_COMMON_PARAM_IS_INVALID);

    noteInit(note);

    json_t *id = json_object_get(json, "id");
    json_t *title = json_object_get(json, "title");
    json_t *content = json_object_get(json, "content");
    json_t *tags = json_object_get(json, "tags");

    if(id && json_is_string(id)) {
        note->id = strdupSafe(json_string_value(id));
    }

    if(title && json_is_string(title)) {
        note->title = strdupSafe(json_string_value(title));
    }

    if(content && json_is_string(content)) {
        note->content = strdupSafe(json_string_value(content));
    }

    if(tags && json_is_array(tags)) {
        size_t count = json_array_size(tags);

        note->tags = calloc(count, sizeof(char *));
        RETURN_ON_COND(!note->tags, RETCODE_COMMON_ALLOC_FAIL);

        for(size_t i = 0; i < count; i++) {
            json_t *tag = json_array_get(tags, i);

            if(!json_is_string(tag)) {
                continue;
            }

            note->tags[i] = strdupSafe(json_string_value(tag));
            RETURN_ON_COND(!note->tags[i], RETCODE_COMMON_STR_DUP_FAIL);

            note->tag_count++;
        }
    }

    return RETCODE_SUCCESS;
}

RetCode noteValidate(const Note *note)
{
    RETURN_ON_COND(!note, RETCODE_COMMON_PARAM_IS_NULL);
    RETURN_ON_COND(!note->title, RETCODE_COMMON_PARAM_IS_NULL);
    RETURN_ON_COND(strlen(note->title) == 0, RETCODE_COMMON_PARAM_IS_INVALID);
    RETURN_ON_COND(!note->content, RETCODE_COMMON_PARAM_IS_NULL);

    return RETCODE_SUCCESS;
}