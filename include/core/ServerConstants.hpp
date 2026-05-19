#pragma once

#include <limits.h>

#ifndef MAX_FDS
#  ifdef OPEN_MAX
#    define MAX_FDS OPEN_MAX
#  elif defined(FD_SETSIZE)
#    define MAX_FDS FD_SETSIZE
#  else
#    define MAX_FDS 256
#  endif
#endif

const int	BUFFER_SIZE = 4096;
const int	PAUSE = 0;
const int	POLL_TIMEOUT = 5000;
const int	CGI_TIMEOUT = 3;
const int	TIMEOUT = 45;

enum HttpStatus
{
    HTTP_CLIENT_DISCONNECTED    = 0,
    HTTP_OK                     = 200,
    HTTP_CREATED                = 201,
    HTTP_NO_CONTENT             = 204,
    HTTP_MOVED_PERMANENTLY      = 301,
    HTTP_FOUND                  = 302,
    HTTP_BAD_REQUEST            = 400,
    HTTP_FORBIDDEN              = 403,
    HTTP_NOT_FOUND              = 404,
    HTTP_METHOD_NOT_ALLOWED     = 405,
    HTTP_REQUEST_TIMEOUT        = 408,
    HTTP_CONTENT_TOO_LARGE      = 413,
    HTTP_INTERNAL_SERVER_ERROR  = 500,
    HTTP_NOT_IMPLEMENTED        = 501,
    HTTP_BAD_GATEWAY            = 502,
    HTTP_GATEWAY_TIMEOUT        = 504
};
