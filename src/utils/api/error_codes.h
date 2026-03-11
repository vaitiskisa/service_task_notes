/**
 * @file error_codes.h
 * @brief Common return codes and error-handling macros.
 */
#ifndef UTILS_ERROR_CODES_H
#define UTILS_ERROR_CODES_H

#include <stdint.h>
#include <stdio.h>

/** Return code type used across modules. */
typedef uint32_t RetCode;

/** Success return code. */
#define RETCODE_OK 0x00000000u

/** @name Module id prefixes */
/**@{*/
#define RETCODE_MODULE_COMMON           (0x0000u << 16)
#define RETCODE_MODULE_HTTP             (0x0001u << 16)
#define RETCODE_MODULE_NOTES_MODEL      (0x0010u << 16)
#define RETCODE_MODULE_NOTES_REPOSITORY (0x0011u << 16)
#define RETCODE_MODULE_NOTES_SERVICE    (0x0012u << 16)
/**@}*/

/** @name Common errors */
/**@{*/
#define RETCODE_COMMON_ERROR           (RETCODE_MODULE_COMMON | 0x0001u)
#define RETCODE_COMMON_NULL_ARG        (RETCODE_MODULE_COMMON | 0x0002u)
#define RETCODE_COMMON_INVALID_ARG     (RETCODE_MODULE_COMMON | 0x0003u)
#define RETCODE_COMMON_NO_MEMORY       (RETCODE_MODULE_COMMON | 0x0004u)
#define RETCODE_COMMON_NOT_INITIALIZED (RETCODE_MODULE_COMMON | 0x0005u)
#define RETCODE_COMMON_ALREADY_EXISTS  (RETCODE_MODULE_COMMON | 0x0006u)
/**@}*/

/** @name HTTP errors */
/**@{*/
#define RETCODE_HTTP_INVALID_REQUEST    (RETCODE_MODULE_HTTP | 0x0001u)
#define RETCODE_HTTP_METHOD_NOT_ALLOWED (RETCODE_MODULE_HTTP | 0x0002u)
#define RETCODE_HTTP_HANDLER_MISSING    (RETCODE_MODULE_HTTP | 0x0003u)
#define RETCODE_HTTP_SERIALIZATION      (RETCODE_MODULE_HTTP | 0x0004u)
/**@}*/

/** @name Notes model errors */
/**@{*/
#define RETCODE_NOTES_INVALID    (RETCODE_MODULE_NOTES_MODEL | 0x0001u)
#define RETCODE_NOTES_JSON_ERROR (RETCODE_MODULE_NOTES_MODEL | 0x0002u)
/**@}*/

/** @name Repository errors */
/**@{*/
#define RETCODE_NOTES_REPOSITORY_INVALID_ARG    (RETCODE_MODULE_NOTES_REPOSITORY | 0x0001u)
#define RETCODE_NOTES_REPOSITORY_NO_MEMORY      (RETCODE_MODULE_NOTES_REPOSITORY | 0x0002u)
#define RETCODE_NOTES_REPOSITORY_IO_ERROR       (RETCODE_MODULE_NOTES_REPOSITORY | 0x0003u)
#define RETCODE_NOTES_REPOSITORY_JSON_ERROR     (RETCODE_MODULE_NOTES_REPOSITORY | 0x0004u)
#define RETCODE_NOTES_REPOSITORY_NOT_FOUND      (RETCODE_MODULE_NOTES_REPOSITORY | 0x0005u)
#define RETCODE_NOTES_REPOSITORY_ALREADY_EXISTS (RETCODE_MODULE_NOTES_REPOSITORY | 0x0006u)
/**@}*/

/** @name Service errors */
/**@{*/
#define RETCODE_NOTES_SERVICE_INVALID_ARG    (RETCODE_MODULE_NOTES_SERVICE | 0x0001u)
#define RETCODE_NOTES_SERVICE_VALIDATION     (RETCODE_MODULE_NOTES_SERVICE | 0x0002u)
#define RETCODE_NOTES_SERVICE_NO_MEMORY      (RETCODE_MODULE_NOTES_SERVICE | 0x0003u)
#define RETCODE_NOTES_SERVICE_NOT_FOUND      (RETCODE_MODULE_NOTES_SERVICE | 0x0004u)
#define RETCODE_NOTES_SERVICE_ALREADY_EXISTS (RETCODE_MODULE_NOTES_SERVICE | 0x0005u)
#define RETCODE_NOTES_SERVICE_STORAGE_ERROR  (RETCODE_MODULE_NOTES_SERVICE | 0x0006u)
/**@}*/

/**
 * @brief Log an error code with source location.
 */
#define LOG_ERROR_CODE(code)                                                                                           \
    fprintf(stderr, "File: %s, Line: %d, Error: 0x%08X\n", __FILE__, __LINE__, (unsigned int)(code))

/**
 * @brief Return early if condition is true, logging the error code.
 */
#define RETURN_ON_COND(cond, code)                                                                                     \
    do {                                                                                                               \
        if(cond) {                                                                                                     \
            LOG_ERROR_CODE(code);                                                                                      \
            return code;                                                                                               \
        }                                                                                                              \
    } while(0)

/**
 * @brief Return early if condition is true (no logging).
 */
#define RETURN_ON_COND_(cond, code)                                                                                    \
    do {                                                                                                               \
        if(cond) {                                                                                                     \
            return code;                                                                                               \
        }                                                                                                              \
    } while(0)

/**
 * @brief Jump to EXIT label if condition is true, logging the error code.
 */
#define TO_EXIT_ON_COND(cond, code)                                                                                    \
    do {                                                                                                               \
        if(cond) {                                                                                                     \
            LOG_ERROR_CODE(code);                                                                                      \
            goto EXIT;                                                                                                 \
        }                                                                                                              \
    } while(0)

/**
 * @brief Log error code if expression returns non-OK.
 */
#define LOG_ON_ERROR(cond)                                                                                             \
    do {                                                                                                               \
        if((ret_code = cond) != RETCODE_OK) {                                                                          \
            LOG_ERROR_CODE(ret_code);                                                                                  \
        }                                                                                                              \
    } while(0)

#endif /* UTILS_ERROR_CODES_H */
