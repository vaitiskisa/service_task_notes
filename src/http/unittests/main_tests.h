#ifndef HTTP_MAIN_TESTS_H
#define HTTP_MAIN_TESTS_H

/**
 * @brief Verifies httpResponseFree clears all fields and zeros sizes.
 */
void testResponseFreeResetsFields();

/**
 * @brief Routes GET /notes with tag query to list handler and captures tag.
 */
void testRouterRoutesCollectionGetWithTag();

/**
 * @brief Decodes URL-encoded tag values before passing to handler.
 */
void testRouterDecodesTagValue();

/**
 * @brief Routes GET /notes/{id} to get handler and captures id.
 */
void testRouterRoutesItemGet();

/**
 * @brief Routes PUT /notes/{id} to update handler and captures id.
 */
void testRouterRoutesItemPut();

/**
 * @brief Routes DELETE /notes/{id} to delete handler and captures id.
 */
void testRouterRoutesItemDelete();

/**
 * @brief Returns 404 for unknown paths.
 */
void testRouterRejectsUnknownPath();

/**
 * @brief Returns 500 when required handler is missing.
 */
void testRouterMissingHandlerReturns500();

/**
 * @brief Returns 405 for unsupported methods on a valid path.
 */
void testRouterMethodNotAllowed();

/**
 * @brief Validates server create/destroy with valid config.
 */
void testServerCreateDestroy();

/**
 * @brief Starts server and handles GET with query parameters.
 */
void testServerHandlesGetWithQuery();

/**
 * @brief Starts server and handles PUT with a request body.
 */
void testServerHandlesPutWithBody();

/**
 * @brief Validates create rejects NULL handler or zero port.
 */
void testServerCreateInvalidArgs();

/**
 * @brief Validates start rejects NULL server.
 */
void testServerStartNull();

/**
 * @brief Validates stop rejects server that was never started.
 */
void testServerStopWithoutStart();

/**
 * @brief Validates destroy rejects NULL server.
 */
void testServerDestroyNull();

#endif /* HTTP_MAIN_TESTS_H */
