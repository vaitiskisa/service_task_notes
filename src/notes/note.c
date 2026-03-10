#include "api/note.h"
#include "api/json_utils.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

static RetCode noteReplaceString(char **dst, const char *src)
{
    char *copy = NULL;

    if(src) {
        copy = strdupSafe(src);
        if(!copy) {
            return RETCODE_COMMON_NO_MEMORY;
        }
    }

    free(*dst);
    *dst = copy;

    return RETCODE_OK;
}

RetCode noteInit(Note *note)
{
    RETURN_ON_COND(!note, RETCODE_COMMON_NULL_ARG);

    note->id = NULL;
    note->title = NULL;
    note->content = NULL;
    note->tags = NULL;
    note->tag_count = 0;

    return RETCODE_OK;
}

RetCode noteClearTags(Note *note)
{
    RETURN_ON_COND(!note, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND_(!note->tags, RETCODE_OK);

    for(size_t i = 0; i < note->tag_count; i++) {
        if(note->tags[i]) {
            free(note->tags[i]);
            note->tags[i] = NULL;
        }
    }

    free(note->tags);
    note->tags = NULL;
    note->tag_count = 0;

    return RETCODE_OK;
}

RetCode noteFree(Note *note)
{
    RETURN_ON_COND(!note, RETCODE_COMMON_NULL_ARG);

    RetCode ret_code = RETCODE_OK;

    if(note->id) {
        free(note->id);
        note->id = NULL;
    }
    if(note->title) {
        free(note->title);
        note->title = NULL;
    }
    if(note->content) {
        free(note->content);
        note->content = NULL;
    }
    LOG_ON_ERROR(noteClearTags(note));

    note->id = NULL;
    note->title = NULL;
    note->content = NULL;

    return ret_code;
}

RetCode noteSetId(Note *note, const char *id)
{
    RETURN_ON_COND(!note, RETCODE_COMMON_NULL_ARG);

    return noteReplaceString(&note->id, id);
}

RetCode noteSetTitle(Note *note, const char *title)
{
    RETURN_ON_COND(!note, RETCODE_COMMON_NULL_ARG);

    return noteReplaceString(&note->title, title);
}

RetCode noteSetContent(Note *note, const char *content)
{
    RETURN_ON_COND(!note, RETCODE_COMMON_NULL_ARG);

    return noteReplaceString(&note->content, content);
}

RetCode noteAddTag(Note *note, const char *tag)
{
    RETURN_ON_COND(!note, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(!tag, RETCODE_COMMON_NULL_ARG);

    char **new_tags = realloc(note->tags, sizeof(char *) * (note->tag_count + 1));
    RETURN_ON_COND(!new_tags, RETCODE_COMMON_NO_MEMORY);

    note->tags = new_tags;

    note->tags[note->tag_count] = strdupSafe(tag);
    RETURN_ON_COND(!note->tags[note->tag_count], RETCODE_COMMON_NO_MEMORY);

    note->tag_count++;

    return RETCODE_OK;
}

RetCode noteSetTags(Note *note, char **tags, size_t count)
{
    RETURN_ON_COND(!note, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(!tags, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(count == 0, RETCODE_COMMON_INVALID_ARG);

    for(size_t i = 0; i < note->tag_count; i++) {
        free(note->tags[i]);
    }

    free(note->tags);

    note->tags = NULL;
    note->tag_count = 0;

    note->tags = calloc(count, sizeof(char *));
    RETURN_ON_COND(!note->tags, RETCODE_COMMON_NO_MEMORY);

    for(size_t i = 0; i < count; i++) {
        note->tags[i] = strdupSafe(tags[i]);
        RETURN_ON_COND(!note->tags[i], RETCODE_COMMON_NO_MEMORY);
    }

    note->tag_count = count;

    return RETCODE_OK;
}

RetCode noteClone(Note *dst, const Note *src)
{
    RETURN_ON_COND(!dst, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(!src, RETCODE_COMMON_NULL_ARG);

    Note tmp;
    RETURN_ON_COND(noteInit(&tmp), RETCODE_COMMON_NO_MEMORY);

    TO_EXIT_ON_COND(noteSetId(&tmp, src->id) != RETCODE_OK, RETCODE_COMMON_ERROR);
    TO_EXIT_ON_COND(noteSetTitle(&tmp, src->title) != RETCODE_OK, RETCODE_COMMON_ERROR);
    TO_EXIT_ON_COND(noteSetContent(&tmp, src->content) != RETCODE_OK, RETCODE_COMMON_ERROR);
    TO_EXIT_ON_COND(noteSetTags(&tmp, (char **)src->tags, src->tag_count) != RETCODE_OK, RETCODE_COMMON_ERROR);

    TO_EXIT_ON_COND(noteFree(dst) != RETCODE_OK, RETCODE_COMMON_ERROR);
    *dst = tmp;

    return RETCODE_OK;
EXIT:
    RETURN_ON_COND(noteFree(&tmp), RETCODE_COMMON_ERROR);
    return RETCODE_COMMON_ERROR;
}

int noteHasTag(const Note *note, const char *tag)
{
    if(!note || !tag) {
        return 0;
    }

    for(size_t i = 0; i < note->tag_count; i++) {
        if(note->tags[i] && strcmp(note->tags[i], tag) == 0) {
            return 1;
        }
    }

    return 0;
}

json_t *noteToJson(const Note *note)
{
    if(!note) {
        return NULL;
    }

    json_t *root = json_object();
    if(!root) {
        return NULL;
    }

    json_t *tags = json_array();
    if(!tags) {
        json_decref(root);
        return NULL;
    }

    for(size_t i = 0; i < note->tag_count; i++) {
        json_t *tag = json_string(note->tags[i] ? note->tags[i] : "");
        if(!tag || json_array_append_new(tags, tag) != 0) {
            json_decref(tag);
            json_decref(tags);
            json_decref(root);
            return NULL;
        }
    }

    TO_EXIT_ON_COND(json_object_set_new(root, "id", note->id ? json_string(note->id) : json_null()) != 0,
                    RETCODE_COMMON_ERROR);
    TO_EXIT_ON_COND(json_object_set_new(root, "title", note->title ? json_string(note->title) : json_null()) != 0,
                    RETCODE_COMMON_ERROR);
    TO_EXIT_ON_COND(json_object_set_new(root, "content", note->content ? json_string(note->content) : json_null()) != 0,
                    RETCODE_COMMON_ERROR);
    TO_EXIT_ON_COND(json_object_set_new(root, "tags", tags) != 0, RETCODE_COMMON_ERROR);

    return root;
EXIT:
    json_decref(tags);
    json_decref(root);
    return NULL;
}

RetCode noteFromJson(Note *note, const json_t *json)
{
    RETURN_ON_COND(!note, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(!json, RETCODE_COMMON_NULL_ARG);
    RETURN_ON_COND(!json_is_object(json), RETCODE_COMMON_INVALID_ARG);

    Note tmp;
    RETURN_ON_COND(noteInit(&tmp), RETCODE_COMMON_NO_MEMORY);

    json_t *id = json_object_get(json, "id");
    json_t *title = json_object_get(json, "title");
    json_t *content = json_object_get(json, "content");
    json_t *tags = json_object_get(json, "tags");

    TO_EXIT_ON_COND(id && !json_is_null(id) && !json_is_string(id), RETCODE_COMMON_INVALID_ARG);
    TO_EXIT_ON_COND(title && !json_is_null(title) && !json_is_string(title), RETCODE_COMMON_INVALID_ARG);
    TO_EXIT_ON_COND(content && !json_is_null(content) && !json_is_string(content), RETCODE_COMMON_INVALID_ARG);
    TO_EXIT_ON_COND(tags && !json_is_array(tags), RETCODE_COMMON_INVALID_ARG);

    TO_EXIT_ON_COND(id && json_is_string(id) && noteSetId(&tmp, json_string_value(id)) != RETCODE_OK,
                    RETCODE_COMMON_ERROR);
    TO_EXIT_ON_COND(title && json_is_string(title) && noteSetTitle(&tmp, json_string_value(title)) != RETCODE_OK,
                    RETCODE_COMMON_ERROR);
    TO_EXIT_ON_COND(content && json_is_string(content) &&
                            noteSetContent(&tmp, json_string_value(content)) != RETCODE_OK,
                    RETCODE_COMMON_ERROR);

    const char **tag_values = NULL;
    if(tags) {
        size_t count = json_array_size(tags);

        if(count > 0) {
            tag_values = calloc(count, sizeof(char *));
            TO_EXIT_ON_COND(!tag_values, RETCODE_COMMON_NO_MEMORY);
        }

        for(size_t i = 0; i < count; i++) {
            json_t *tag = json_array_get(tags, i);
            TO_EXIT_ON_COND(!json_is_string(tag), RETCODE_COMMON_INVALID_ARG);
            tag_values[i] = json_string_value(tag);
        }

        TO_EXIT_ON_COND(noteSetTags(&tmp, (char **)tag_values, count) != RETCODE_OK, RETCODE_COMMON_ERROR);
        free(tag_values);
    }

    TO_EXIT_ON_COND(noteFree(note) != RETCODE_OK, RETCODE_COMMON_ERROR);
    *note = tmp;

    return RETCODE_OK;

EXIT:
    if(tag_values) {
        free(tag_values);
    }

    RETURN_ON_COND(noteFree(&tmp) != RETCODE_OK, RETCODE_COMMON_ERROR);
    return RETCODE_COMMON_ERROR;
}

static inline int noteIsNonemptyString(const char *s)
{
    return (s && *s != '\0');
}

RetCode noteValidateForCreate(const Note *note)
{
    RETURN_ON_COND(!note, RETCODE_COMMON_NULL_ARG);

    RETURN_ON_COND(!noteIsNonemptyString(note->title), RETCODE_NOTES_INVALID);
    RETURN_ON_COND(!noteIsNonemptyString(note->content), RETCODE_NOTES_INVALID);

    return RETCODE_OK;
}

RetCode noteValidateForUpdate(const Note *note)
{
    RETURN_ON_COND(!note, RETCODE_COMMON_NULL_ARG);

    RETURN_ON_COND(!noteIsNonemptyString(note->id), RETCODE_NOTES_INVALID);

    return noteValidateForCreate(note);
}