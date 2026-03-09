#ifndef ERROR_CODES_H
#define ERROR_CODES_H

#include <stdint.h>
#include <stdio.h>

typedef uint32_t RetCode;

#define RETCODE_SUCCESS 0x0

#define RETCODE_COMMON                  (0x0)
#define RETCODE_COMMON_FAIL             (RETCODE_COMMON | 0x01)
#define RETCODE_COMMON_PARAM_IS_NULL    (RETCODE_COMMON | 0x02)
#define RETCODE_COMMON_PARAM_IS_INVALID (RETCODE_COMMON | 0x03)
#define RETCODE_COMMON_ALLOC_FAIL       (RETCODE_COMMON | 0x04)
#define RETCODE_COMMON_STR_DUP_FAIL     (RETCODE_COMMON | 0x05)
#define RETCODE_COMMON_NEED_INIT        (RETCODE_COMMON | 0x06)
#define RETCODE_COMMON_PATH_EXISTS      (RETCODE_COMMON | 0x07)

#define RETCODE_NOTES_REPO           (0x0)
#define RETCODE_NOTES_INVALID_ARG    (RETCODE_NOTES_REPO | 0x01)
#define RETCODE_NOTES_NOMEM          (RETCODE_NOTES_REPO | 0x02)
#define RETCODE_NOTES_IO_FAIL        (RETCODE_NOTES_REPO | 0x03)
#define RETCODE_NOTES_JSON_FAIL      (RETCODE_NOTES_REPO | 0x04)
#define RETCODE_NOTES_NOT_FOUND      (RETCODE_NOTES_REPO | 0x05)
#define RETCODE_NOTES_ALREADY_EXISTS (RETCODE_NOTES_REPO | 0x06)

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

#define LOG_ON_ERROR(cond, code)                                                                                       \
    do {                                                                                                               \
        if((ret_code = cond) != RETCODE_SUCCESS) {                                                                     \
            LOG_ERROR_CODE(code);                                                                                      \
        }                                                                                                              \
    } while(0)

#endif /* ERROR_CODES_H */