#include "api/helper.h"
#include <string.h>
#include <stdlib.h>

char *strdupSafe(const char *s)
{
    if(!s) {
        return NULL;
    }

    size_t len = strlen(s);
    char *copy = malloc(len + 1);
    if(!copy) {
        return NULL;
    }

    memcpy(copy, s, len + 1);
    return copy;
}
