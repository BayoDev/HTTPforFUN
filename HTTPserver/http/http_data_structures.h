#ifndef HTTP_DATA_STRUCT
#define HTTP_DATA_STRUCT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "http_lib.h"

//######################
//       Structs
//######################

typedef enum{
    OK_200,
    NOT_MOD_304,
    BAD_REQ_400,
    NO_AUTH_401,
    FORBIDDEN_403,
    NOT_FOUND_404,
    INTERNAL_ERROR_500,
    NOT_IMPLEMENTED_501
} RESPONSE_CODE;

typedef enum{
    GET,
    HEAD,
    POST,
    NONE        // Not implemented
} METHOD;

struct header_field{
    char* NAME;
    char* VALUE;
    struct header_field* next_field;
};

struct request_data{
    char* METHOD;
    struct path_request_data* PATH_DATA;
    char* PARAMETERS;
    char* VERSION;
    struct header_field* other_fields; 
};

struct response_data{
    char* CODE;
    char* DESC;
    char* VERSION;
    struct header_field* other_fields;
};

struct path_request_data{
    char* full_path;
    char* real_full_path;
    char* requested_path;
    char* filename;
    char* parameters;
    char* file_extension;
};

struct server_config_data{
    char* ROOT_FOLDER;
    char* ROOT_FOLDER_REAL_PATH;
    char* PORT;
    char* HTTP_VERSION;
    char* SERVER_ID;
};

//######################
//      Operations
//######################

// Server config ops
struct server_config_data* init_server_config(char* ROOT_FOLDER,char* PORT,char* HTTP_VERSION,char* SERVER_ID);

// Request operations
struct request_data* init_request(char* METHOD, struct path_request_data* PATH_DATA,char* VERSION);
void free_request_data(struct request_data* st);
void print_request(struct request_data rq);

// Response operations
struct response_data* init_response(char* CODE,char* DESC,char* VERSION);
void update_response(char* CODE,char* DESC,char* VERSION,struct response_data* rd_data);
void free_response_data(struct response_data* st);
void print_response(struct response_data rd);

// Header operations
void header_add_field_req(char* NAME,char* VALUE,struct request_data* st);
void header_add_field_resp(char* NAME,char* VALUE,struct response_data* st);
void free_header_field(struct header_field* hd);

// Path request operations
struct path_request_data* init_path_data(char* request_path,struct server_config_data* server_info);
void print_path_data(struct path_request_data* pr_data);
void free_path_data(struct path_request_data* pr_data);

// Helpers
char* resp_to_str(struct response_data rd);
char* header_to_str(struct header_field rd);

#endif