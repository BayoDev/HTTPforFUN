#ifndef CGI_INTERFACE
#define CGI_INTERFACE

#include "../http/http_data_structures.h"
#include "../http/http_lib.h"
#include "../http/http_config.h"

#include <stdlib.h>

void prepare_cgi_env(struct request_data* rq_data,struct server_config_data* server_info);
bool spawn_cgi_process(char* filename,int file_fd,int DATA_SOCKET);

#endif