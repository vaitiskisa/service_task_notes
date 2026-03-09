#include "api/response.h"

#include <stdlib.h>

RetCode httpResponseFree(HttpResponse *response)
{
    RETURN_ON_COND(!response, RETCODE_COMMON_PARAM_IS_NULL);

    free(response->content_type);
    free(response->body);

    response->content_type = NULL;
    response->body = NULL;
    response->body_size = 0;
    response->status_code = 0;

    return RETCODE_SUCCESS;
}