/**
 * @file notes_handler.h
 * @brief HTTP handlers for the notes API.
 */
#ifndef NOTES_HANDLER_API_H
#define NOTES_HANDLER_API_H

#include "api/request.h"
#include "api/response.h"
#include "api/notes_service.h"
#include "api/error_codes.h"

/**
 * @brief Handler context shared by notes API endpoints.
 */
typedef struct NotesHandlerContext {
    /** Notes service instance used by handlers. */
    NotesService *service;
} NotesHandlerContext;

/** @name Context management */
/**@{*/
/**
 * @brief Allocate a handler context.
 *
 * @param service Notes service instance.
 * @return New context or NULL on error.
 */
NotesHandlerContext *createNotesHandlerContext(NotesService *service);
/**
 * @brief Destroy a handler context.
 *
 * @param ctx Context to destroy (may be NULL).
 * @return RETCODE_OK on success.
 */
RetCode destroyNotesHandlerContext(NotesHandlerContext *ctx);
/**@}*/

/** @name HTTP handlers */
/**@{*/
/**
 * @brief Handle POST /notes.
 *
 * @param request HTTP request.
 * @param userData Handler context.
 * @return HTTP response.
 */
HttpResponse createNoteHandler(const HttpRequest *request, void *userData);
/**
 * @brief Handle GET /notes (optionally filtered by tag).
 *
 * @param request HTTP request.
 * @param tag Optional tag filter.
 * @param userData Handler context.
 * @return HTTP response.
 */
HttpResponse listNotesHandler(const HttpRequest *request, const char *tag, void *userData);
/**
 * @brief Handle GET /notes/{id}.
 *
 * @param request HTTP request.
 * @param noteId Note identifier.
 * @param userData Handler context.
 * @return HTTP response.
 */
HttpResponse getNoteHandler(const HttpRequest *request, const char *noteId, void *userData);
/**
 * @brief Handle PUT /notes/{id}.
 *
 * @param request HTTP request.
 * @param noteId Note identifier.
 * @param userData Handler context.
 * @return HTTP response.
 */
HttpResponse updateNoteHandler(const HttpRequest *request, const char *noteId, void *userData);
/**
 * @brief Handle DELETE /notes/{id}.
 *
 * @param request HTTP request.
 * @param noteId Note identifier.
 * @param userData Handler context.
 * @return HTTP response.
 */
HttpResponse deleteNoteHandler(const HttpRequest *request, const char *noteId, void *userData);
/**@}*/

#endif /* NOTES_HANDLER_API_H */
