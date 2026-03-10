#ifndef ERROR_CODES_H
#define ERROR_CODES_H

#include <stdint.h>
#include <stdio.h>

typedef uint32_t RetCode;

#define RETCODE_OK 0x00000000u

#define RETCODE_MODULE_COMMON           (0x0000u << 16)
#define RETCODE_MODULE_HTTP             (0x0001u << 16)
#define RETCODE_MODULE_NOTES_MODEL      (0x0010u << 16)
#define RETCODE_MODULE_NOTES_REPOSITORY (0x0011u << 16)
#define RETCODE_MODULE_NOTES_SERVICE    (0x0012u << 16)

#define RETCODE_COMMON_ERROR           (RETCODE_MODULE_COMMON | 0x0001u)
#define RETCODE_COMMON_NULL_ARG        (RETCODE_MODULE_COMMON | 0x0002u)
#define RETCODE_COMMON_INVALID_ARG     (RETCODE_MODULE_COMMON | 0x0003u)
#define RETCODE_COMMON_NO_MEMORY       (RETCODE_MODULE_COMMON | 0x0004u)
#define RETCODE_COMMON_NOT_INITIALIZED (RETCODE_MODULE_COMMON | 0x0005u)
#define RETCODE_COMMON_ALREADY_EXISTS  (RETCODE_MODULE_COMMON | 0x0006u)

#define RETCODE_HTTP_INVALID_REQUEST    (RETCODE_MODULE_HTTP | 0x0001u)
#define RETCODE_HTTP_METHOD_NOT_ALLOWED (RETCODE_MODULE_HTTP | 0x0002u)
#define RETCODE_HTTP_HANDLER_MISSING    (RETCODE_MODULE_HTTP | 0x0003u)
#define RETCODE_HTTP_SERIALIZATION      (RETCODE_MODULE_HTTP | 0x0004u)

#define RETCODE_NOTES_INVALID    (RETCODE_MODULE_NOTES_MODEL | 0x0001u)
#define RETCODE_NOTES_JSON_ERROR (RETCODE_MODULE_NOTES_MODEL | 0x0002u)

#define RETCODE_NOTES_REPOSITORY_INVALID_ARG    (RETCODE_MODULE_NOTES_REPOSITORY | 0x0001u)
#define RETCODE_NOTES_REPOSITORY_NO_MEMORY      (RETCODE_MODULE_NOTES_REPOSITORY | 0x0002u)
#define RETCODE_NOTES_REPOSITORY_IO_ERROR       (RETCODE_MODULE_NOTES_REPOSITORY | 0x0003u)
#define RETCODE_NOTES_REPOSITORY_JSON_ERROR     (RETCODE_MODULE_NOTES_REPOSITORY | 0x0004u)
#define RETCODE_NOTES_REPOSITORY_NOT_FOUND      (RETCODE_MODULE_NOTES_REPOSITORY | 0x0005u)
#define RETCODE_NOTES_REPOSITORY_ALREADY_EXISTS (RETCODE_MODULE_NOTES_REPOSITORY | 0x0006u)

#define RETCODE_NOTES_SERVICE_INVALID_ARG    (RETCODE_MODULE_NOTES_SERVICE | 0x0001u)
#define RETCODE_NOTES_SERVICE_VALIDATION     (RETCODE_MODULE_NOTES_SERVICE | 0x0002u)
#define RETCODE_NOTES_SERVICE_NO_MEMORY      (RETCODE_MODULE_NOTES_SERVICE | 0x0003u)
#define RETCODE_NOTES_SERVICE_NOT_FOUND      (RETCODE_MODULE_NOTES_SERVICE | 0x0004u)
#define RETCODE_NOTES_SERVICE_ALREADY_EXISTS (RETCODE_MODULE_NOTES_SERVICE | 0x0005u)
#define RETCODE_NOTES_SERVICE_STORAGE_ERROR  (RETCODE_MODULE_NOTES_SERVICE | 0x0006u)

#define LOG_ERROR_CODE(code)                                                                                           \
    fprintf(stderr, "File: %s, Line: %d, Error: 0x%08X\n", __FILE__, __LINE__, (unsigned int)(code))

#define RETURN_ON_COND(cond, code)                                                                                     \
    do {                                                                                                               \
        if(cond) {                                                                                                     \
            LOG_ERROR_CODE(code);                                                                                      \
            return code;                                                                                               \
        }                                                                                                              \
    } while(0)

#define RETURN_ON_COND_(cond, code)                                                                                    \
    do {                                                                                                               \
        if(cond) {                                                                                                     \
            return code;                                                                                               \
        }                                                                                                              \
    } while(0)

#define TO_EXIT_ON_COND(cond, code)                                                                                    \
    do {                                                                                                               \
        if(cond) {                                                                                                     \
            LOG_ERROR_CODE(code);                                                                                      \
            goto EXIT;                                                                                                 \
        }                                                                                                              \
    } while(0)

#define LOG_ON_ERROR(cond)                                                                                             \
    do {                                                                                                               \
        if((ret_code = cond) != RETCODE_OK) {                                                                          \
            LOG_ERROR_CODE(ret_code);                                                                                  \
        }                                                                                                              \
    } while(0)

#endif /* ERROR_CODES_H */