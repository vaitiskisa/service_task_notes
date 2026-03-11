/**
 * @file router.h
 * @brief HTTP router for notes endpoints.
 */
#ifndef HTTP_ROUTER_H
#define HTTP_ROUTER_H

#include "api/error_codes.h"
#include "request.h"
#include "response.h"

/** Opaque router handle. */
typedef struct HttpRouter HttpRouter;

/** @name Notes endpoint callback types */
/**@{*/
/**
 * @brief Generic route handler type.
 */
typedef HttpResponse (*HttpRouteHandler)(const HttpRequest *request, void *user_data);
/** @brief Handler for POST /notes. */
typedef HttpResponse (*NotesCreateHandler)(const HttpRequest *request, void *user_data);
/** @brief Handler for GET /notes. */
typedef HttpResponse (*NotesListHandler)(const HttpRequest *request, const char *tag, void *user_data);
/** @brief Handler for GET /notes/{id}. */
typedef HttpResponse (*NotesGetHandler)(const HttpRequest *request, const char *note_id, void *user_data);
/** @brief Handler for PUT /notes/{id}. */
typedef HttpResponse (*NotesUpdateHandler)(const HttpRequest *request, const char *note_id, void *user_data);
/** @brief Handler for DELETE /notes/{id}. */
typedef HttpResponse (*NotesDeleteHandler)(const HttpRequest *request, const char *note_id, void *user_data);
/**@}*/

/**
 * @brief Router configuration.
 */
typedef struct HttpRouterConfig {
    /** Create note handler. */
    NotesCreateHandler create_note;
    /** List notes handler. */
    NotesListHandler list_notes;
    /** Get note handler. */
    NotesGetHandler get_note;
    /** Update note handler. */
    NotesUpdateHandler update_note;
    /** Delete note handler. */
    NotesDeleteHandler delete_note;
    /** User data passed to handlers. */
    void *notes_user_data;
} HttpRouterConfig;

/** @name Router lifecycle */
/**@{*/
/**
 * @brief Create a router instance.
 *
 * @param config Router configuration.
 * @return Router handle or NULL on error.
 */
HttpRouter *httpRouterCreate(const HttpRouterConfig *config);
/**
 * @brief Destroy a router instance.
 *
 * @param router Router handle.
 * @return RETCODE_OK on success, otherwise an error code.
 */
RetCode httpRouterDestroy(HttpRouter *router);
/**@}*/

/** @name Routing */
/**@{*/
/**
 * @brief Main router entrypoint for the HTTP server.
 *
 * Matches @ref HttpRouteHandler and can be used as the server handler.
 *
 * @param request HTTP request.
 * @param user_data Router instance.
 * @return HTTP response.
 */
HttpResponse httpRouterHandle(const HttpRequest *request, void *user_data);
/**@}*/

#endif /* HTTP_ROUTER_H */
