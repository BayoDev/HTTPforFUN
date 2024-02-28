#ifndef HTTP_LIB
#define HTTP_LIB

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>                                                                                                
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "http_config.h"

#ifdef DEBUG
#define debug(...) printf(__VA_ARGS__);
#else
#define debug(...)
#endif

void* handle_request(void* socket_fd);

#endif