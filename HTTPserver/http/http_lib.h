#ifndef HTTP_LIB
#define HTTP_LIB

// Needed for strptime
#define _XOPEN_SOURCE

// Needed for getline
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>                                                                                                
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#include <time.h>

#include "http_config.h"
#include "http_data_structures.h"
#include "../cgi/cgi_interface.h"
#include "../c_log/c_log.h"

void* handle_request(void* socket_fd,struct server_config_data* server_info);
char *adapt_filename(char* file,struct server_config_data* server_info);

#endif