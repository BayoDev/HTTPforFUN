#ifndef HTTP_DATA_STRUCT
#define HTTP_DATA_STRUCT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//######################
//       Structs
//######################

typedef enum{
    OK_200,
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
    char* FILE;
    char* VERSION;
    struct header_field* other_fields; 
};

struct response_data{
    char* CODE;
    char* DESC;
    char* VERSION;
    struct header_field* other_fields;
};

//######################
//      Operations
//######################

// Request operations
struct request_data* init_request(char* METHOD,char* FILE,char* VERSION);
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

// Helpers
char* resp_to_str(struct response_data rd);
char* header_to_str(struct header_field rd);

#endif