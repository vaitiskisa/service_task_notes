#ifndef HTTP_MOCK_FUNCTIONS_H
#define HTTP_MOCK_FUNCTIONS_H

#include "api/response.h"
#include "api/request.h"

typedef struct HandlerCapture {
    /** Captured tag value. */
    char tag[128];
    /** Captured note id. */
    char note_id[128];
    /** Whether create handler was called. */
    int called_create;
    /** Whether list handler was called. */
    int called_list;
    /** Whether get handler was called. */
    int called_get;
    /** Whether update handler was called. */
    int called_update;
    /** Whether delete handler was called. */
    int called_delete;
} HandlerCapture;

/**
 * @brief Reset handler capture state to defaults.
 *
 * @param cap Capture struct to reset.
 */
void resetCapture(HandlerCapture *cap);

/**
 * @brief Mock handler for POST /notes.
 *
 * Sets create flag and returns a 200 JSON response.
 */
HttpResponse handleCreate(const HttpRequest *request, void *user_data);

/**
 * @brief Mock handler for GET /notes.
 *
 * Captures tag (if provided) and returns a 200 JSON response.
 */
HttpResponse handleList(const HttpRequest *request, const char *tag, void *user_data);

/**
 * @brief Mock handler for GET /notes/{id}.
 *
 * Captures note id and returns a 200 JSON response.
 */
HttpResponse handleGet(const HttpRequest *request, const char *note_id, void *user_data);

/**
 * @brief Mock handler for PUT /notes/{id}.
 *
 * Captures note id and returns a 200 JSON response.
 */
HttpResponse handleUpdate(const HttpRequest *request, const char *note_id, void *user_data);

/**
 * @brief Mock handler for DELETE /notes/{id}.
 *
 * Captures note id and returns a 200 JSON response.
 */
HttpResponse handleDelete(const HttpRequest *request, const char *note_id, void *user_data);

/**
 * @brief Mock handler that performs no capture.
 *
 * Returns a 200 JSON response.
 */
HttpResponse handleNoop(const HttpRequest *request, void *user_data);

#endif /* HTTP_MOCK_FUNCTIONS_H */
