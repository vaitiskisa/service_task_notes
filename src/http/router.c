#include "api/router.h"
#include "api/json_utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>

#define ERROR_BUFFER_SIZE 256

/*
 * Route classification:
 *
 *   /notes          -> collection endpoint
 *   /notes/         -> collection endpoint
 *   /notes/{id}     -> item endpoint
 *
 * Reject:
 *   /notes/{id}/x
 *   /note
 *   /somethingelse
 */
typedef enum RoutePathKind {
    ROUTE_PATH_START = 0,
    ROUTE_PATH_INVALID = ROUTE_PATH_START,
    ROUTE_PATH_NOTES_COLLECTION,
    ROUTE_PATH_NOTES_ITEM,
    ROUTE_PATH_TOT,
} RoutePathKind;

typedef struct RoutePathInfo {
    RoutePathKind kind;
    char *note_id; /* only set for ROUTE_PATH_NOTES_ITEM */
} RoutePathInfo;

struct HttpRouter {
    HttpRouterConfig config;
};

static inline int isMethod(const HttpRequest *request, const char *method)
{
    return request && request->method && method && strcmp(request->method, method) == 0;
}

static int hexValue(char c)
{
    if(c >= '0' && c <= '9') {
        return c - '0';
    }
    if(c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    if(c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }
    return -1;
}

static char *urlDecode(const char *src)
{
    if(!src) {
        return NULL;
    }

    size_t len = strlen(src);
    char *dst = malloc(len + 1);
    if(!dst) {
        return NULL;
    }

    size_t i = 0;
    size_t j = 0;

    while(i < len) {
        if(src[i] == '+') {
            dst[j++] = ' ';
            i++;
        } else if(src[i] == '%' && i + 2 < len) {
            int hi = hexValue(src[i + 1]);
            int lo = hexValue(src[i + 2]);

            if(hi >= 0 && lo >= 0) {
                dst[j++] = (char)((hi << 4) | lo);
                i += 3;
            } else {
                dst[j++] = src[i++];
            }
        } else {
            dst[j++] = src[i++];
        }
    }

    dst[j] = '\0';
    return dst;
}

/*
 * Extract raw query parameter value for a given key from a query string like:
 *   "tag=work&x=1"
 *
 * Returns heap-allocated URL-decoded value, or NULL if not found / invalid.
 */
static char *queryGetParam(const char *query, const char *key)
{
    if(!query || !key || *key == '\0') {
        return NULL;
    }

    size_t key_len = strlen(key);
    const char *p = query;

    while(*p != '\0') {
        const char *pair_start = p;
        const char *pair_end = strchr(pair_start, '&');
        if(!pair_end) {
            pair_end = pair_start + strlen(pair_start);
        }

        const char *eq = memchr(pair_start, '=', (size_t)(pair_end - pair_start));

        if(eq) {
            size_t current_key_len = (size_t)(eq - pair_start);

            if(current_key_len == key_len && strncmp(pair_start, key, key_len) == 0) {
                size_t value_len = (size_t)(pair_end - eq - 1);

                char *raw_value = malloc(value_len + 1);
                if(!raw_value) {
                    return NULL;
                }

                memcpy(raw_value, eq + 1, value_len);
                raw_value[value_len] = '\0';

                char *decoded = urlDecode(raw_value);
                free(raw_value);
                return decoded;
            }
        } else {
            size_t current_key_len = (size_t)(pair_end - pair_start);

            if(current_key_len == key_len && strncmp(pair_start, key, key_len) == 0) {
                return strdupSafe("");
            }
        }

        p = (*pair_end == '&') ? pair_end + 1 : pair_end;
    }

    return NULL;
}

static RetCode routePathInfoFree(RoutePathInfo *info)
{
    RETURN_ON_COND(!info, RETCODE_COMMON_NULL_ARG);

    free(info->note_id);
    info->note_id = NULL;
    info->kind = ROUTE_PATH_INVALID;

    return RETCODE_OK;
}

static RoutePathInfo parseNotesPath(const char *path)
{
    RoutePathInfo info = { 0 };

    if(!path) {
        return info;
    }

    if(strcmp(path, "/notes") == 0 || strcmp(path, "/notes/") == 0) {
        info.kind = ROUTE_PATH_NOTES_COLLECTION;
        return info;
    }

    const char *prefix = "/notes/";
    size_t prefix_len = strlen(prefix);

    if(strncmp(path, prefix, prefix_len) != 0) {
        return info;
    }

    const char *id_part = path + prefix_len;

    if(*id_part == '\0') {
        info.kind = ROUTE_PATH_NOTES_COLLECTION;
        return info;
    }

    if(strchr(id_part, '/') != NULL) {
        return info;
    }

    info.note_id = strdupSafe(id_part);
    if(!info.note_id) {
        return info;
    }

    info.kind = ROUTE_PATH_NOTES_ITEM;
    return info;
}

HttpRouter *httpRouterCreate(const HttpRouterConfig *config)
{
    if(!config) {
        return NULL;
    }

    HttpRouter *router = calloc(1, sizeof(*router));
    if(!router) {
        return NULL;
    }

    router->config = *config;
    return router;
}

RetCode httpRouterDestroy(HttpRouter *router)
{
    RETURN_ON_COND(!router, RETCODE_COMMON_NULL_ARG);

    free(router);

    return RETCODE_OK;
}

static HttpResponse dispatchCollection(HttpRouter *router, const HttpRequest *request)
{
    if(isMethod(request, "POST")) {
        if(!router->config.create_note) {
            return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "Create note handler not configured");
        }
        return router->config.create_note(request, router->config.notes_user_data);
    }

    if(isMethod(request, "GET")) {
        printf("Handling GET /notes with query: %s\n", request->query ? request->query : "(null)");
        if(!router->config.list_notes) {
            return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "List notes handler not configured");
        }

        char *tag = queryGetParam(request->query, "tag");
        HttpResponse response = router->config.list_notes(request, tag, router->config.notes_user_data);
        free(tag);
        return response;
    }

    return makeErrorResponse(HTTP_STATUS_METHOD_NOT_ALLOWED, "Method not allowed");
}

static HttpResponse dispatchItem(HttpRouter *router, const HttpRequest *request, const char *note_id)
{
    if(!note_id || *note_id == '\0') {
        return makeErrorResponse(HTTP_STATUS_BAD_REQUEST, "Invalid note id");
    }

    if(isMethod(request, "GET")) {
        if(!router->config.get_note) {
            return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "Get note handler not configured");
        }
        return router->config.get_note(request, note_id, router->config.notes_user_data);
    }

    if(isMethod(request, "PUT")) {
        if(!router->config.update_note) {
            return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "Update note handler not configured");
        }
        return router->config.update_note(request, note_id, router->config.notes_user_data);
    }

    if(isMethod(request, "DELETE")) {
        if(!router->config.delete_note) {
            return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "Delete note handler not configured");
        }
        return router->config.delete_note(request, note_id, router->config.notes_user_data);
    }

    return makeErrorResponse(HTTP_STATUS_METHOD_NOT_ALLOWED, "Method not allowed");
}

HttpResponse httpRouterHandle(const HttpRequest *request, void *user_data)
{
    HttpRouter *router = (HttpRouter *)user_data;

    if(!router || !request || !request->path) {
        return makeErrorResponse(HTTP_STATUS_INTERNAL_ERROR, "Router misconfiguration");
    }

    RoutePathInfo route = parseNotesPath(request->path);
    HttpResponse response = { 0 };

    switch(route.kind) {
    case ROUTE_PATH_NOTES_COLLECTION:
        response = dispatchCollection(router, request);
        break;

    case ROUTE_PATH_NOTES_ITEM:
        response = dispatchItem(router, request, route.note_id);
        break;

    case ROUTE_PATH_INVALID:
    default:
        response = makeErrorResponse(HTTP_STATUS_NOT_FOUND, "Not found");
        break;
    }

    RetCode ret_code = RETCODE_OK;
    LOG_ON_ERROR(routePathInfoFree(&route));

    return response;
}