#include "http_lib.h"

typedef enum{
    OK_200,
    BAD_REQ_400,
    NO_AUTH_401,
    FORBIDDEN_403,
    NOT_FOUND_404,
    INTERNAL_ERROR_500,
    NOT_IMPLEMENTED_501
} RESPONSE_CODE;

//=====================
//      AUXILIARY
//=====================

char *adapt_filename(char* file){
    if(strlen(file)==0){file=malloc(sizeof(char)*2);file="/";}

    // Sanitazation
    char* aux = file;
    for(;;){
        char* occurrence = strpbrk(aux,".");
        if(occurrence==NULL) break;
        if(*(occurrence+1)=='.'){ file = occurrence+2; aux=file;}
        else{aux=occurrence+1;}
    }

    // TODO add error control to malloc
    char *full_filename = malloc(sizeof(char)*(strlen(ROOT_FOLDER)+strlen(file)+strlen("index.html")+1));
    strcpy(full_filename,ROOT_FOLDER);
    strcat(full_filename,file);
    if(file[strlen(file)-1]=='/') strcat(full_filename,"index.html");
    return full_filename;
}

//=====================
//   DATA STRUCTURES
//=====================

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

struct request_data* init_request(char* METHOD,char* FILE,char* VERSION){
    if(METHOD==NULL || FILE==NULL || VERSION==NULL) return NULL;
    struct request_data* new = malloc(sizeof(struct request_data));

    new->METHOD = malloc(sizeof(char)*(strlen(METHOD)+1));
    strcpy(new->METHOD,METHOD);
    new->FILE = malloc(sizeof(char)*(strlen(FILE)+1));
    strcpy(new->FILE,FILE);
    new->VERSION = malloc(sizeof(char)*(strlen(VERSION)+1));
    strcpy(new->VERSION,VERSION); 

    new->other_fields = NULL;
    return new;
}

void request_add_field(char* NAME,char* VALUE,struct request_data* st){
    // Find placement
    struct header_field* aux = st->other_fields;
    while(aux!=NULL && aux->next_field!=NULL) aux=aux->next_field;

    // Place struct
    struct header_field* new = malloc(sizeof(struct header_field));
    
    new->NAME = malloc(sizeof(char)*(strlen(NAME)+1));
    strcpy(new->NAME,NAME);

    new->VALUE = malloc(sizeof(char)*(strlen(VALUE)+1));
    strcpy(new->VALUE,VALUE);

    new->next_field = NULL;

    if(aux==NULL) st->other_fields = new;
    else aux->next_field=new;
    return;
}

void print_request(struct request_data rq){
    printf("%s %s %s\n",rq.METHOD,rq.FILE,rq.VERSION);
    struct header_field* aux = rq.other_fields;
    while(aux!=NULL){
        printf("%s : %s\n",aux->NAME,aux->VALUE);
        aux=aux->next_field;
    }
}

// TODO do this without recursion
void free_header_field(struct header_field* hd){
    if(hd==NULL || hd->next_field==NULL) return;
    free_header_field(hd->next_field);
    free(hd->next_field->NAME);
    free(hd->next_field->VALUE);
    free(hd->next_field);
}

void free_request_data(struct request_data* st){
    if(st->other_fields!=NULL){
        free_header_field(st->other_fields);
        free(st->other_fields->NAME);
        free(st->other_fields->VALUE);
        free(st->other_fields);
    }
    free(st->FILE);
    free(st->METHOD);
    free(st->VERSION);
    free(st);
}

//=====================
//     HANDLE I/O
//=====================

// This function return parse the request, returns a response code and the request data into rt
RESPONSE_CODE parse_request(FILE* input_stream,struct request_data** rt){
    if(input_stream==NULL) return INTERNAL_ERROR_500;
    *rt = NULL;
    char* line_buffer;
    ssize_t read_val;
    size_t line_size = 0;
    RESPONSE_CODE ret_code = OK_200;
    // Read the first header line
    {
        read_val = getline(&line_buffer,&line_size,input_stream);
        if(read_val==-1){ 
            free(line_buffer);
            perror("Something went wrong while receiving a request header\n");
            return INTERNAL_ERROR_500;
        }
        debug("[DEBUG] Received request: %s",line_buffer);
        line_size=0;
    }

    // TODO change for thread safety
    char* method = strtok(line_buffer," ");
    char* file = strtok(NULL," ");
    char* version = strtok(NULL,"\r\n");

    // Check this data for validity
    if((strcmp(method,"GET")!=0)&&(strcmp(method,"HEAD")!=0)&&(strcmp(method,"POST")!=0)) ret_code = INTERNAL_ERROR_500;
    // TODO check for valid version
    char* full_filename = adapt_filename(file);
    (*rt) = init_request(method,full_filename,version);

    free(full_filename);
    free(line_buffer);

    for(;;){
        read_val = getline(&line_buffer,&line_size,input_stream);
        if(read_val==-1){ free(line_buffer); free(*rt); return INTERNAL_ERROR_500; }
        if(strcmp(line_buffer,"\r\n")==0) break;

        // TODO make this thread-safe
        char* NAME = strtok(line_buffer,":");
        char* VALUE = strtok(NULL,"\r\n");
        
        request_add_field(NAME,VALUE+1,(*rt));

        free(line_buffer);
        line_size = 0;
    }
    free(line_buffer);

    print_request(**rt);

    // Get file data
    struct stat file_stat;
    if(stat((*rt)->FILE,&file_stat)==-1){
        switch(errno){
            case ENOENT:
                ret_code = NOT_FOUND_404;
                break;
            case EACCES:
                ret_code = FORBIDDEN_403;
                break;
            default:
                ret_code = INTERNAL_ERROR_500;
        }
    }

    return ret_code;
}

void send_response(RESPONSE_CODE code,struct request_data* rq_data,FILE* conn_stream, int socket_fd){

    // TODO do this better
    char HEADER_CONTENT[500];
    switch(code){
        case OK_200:
            strcpy(HEADER_CONTENT,"HTTP/1.0 200 OK\r\n\r\n");
            break;
        case FORBIDDEN_403: 
            strcpy(HEADER_CONTENT,"HTTP/1.0 403 Forbidden\r\n\r\n");
            free(rq_data->FILE);
            rq_data->FILE = adapt_filename(FILE_403);
            break;
        case NOT_FOUND_404:
            strcpy(HEADER_CONTENT,"HTTP/1.0 404 Not Found\r\n\r\n");
            free(rq_data->FILE);
            rq_data->FILE = adapt_filename(FILE_404);
            break;
        default:
            strcpy(HEADER_CONTENT,"HTTP/1.0 500 Internal Server error\r\n\r\n");
            free(rq_data->FILE);
            rq_data->FILE = adapt_filename(FILE_500);
            break;
    }

    debug("[DEBUG] Sending file: '%s' with header: %s\n",rq_data->FILE,HEADER_CONTENT);

    // Open file that needs to be sent
    int file_fd = open(rq_data->FILE,O_RDONLY,NULL);
    if(file_fd==-1){
        if(errno==EACCES && code!=FORBIDDEN_403){
            send_response(FORBIDDEN_403,rq_data,conn_stream,socket_fd);
            return;
        }
        debug("Something went wrong with open while sending response\n");
        return;
    }

    // Get stat of file to get size of file
    struct stat file_stat;
    if(fstat(file_fd,&file_stat)==-1){
        perror("Something went wrong in 'stat' call while sending response\n");
        close(file_fd);
        return;
    }

    // Send first header line
    fputs(HEADER_CONTENT,conn_stream);
    fseek(conn_stream,0L,SEEK_CUR);                         // sync

    // Transmit file data
    off_t file_size = file_stat.st_size;
    off_t offset = 0;
    while(file_size!=0){
        int sent_data = sendfile(socket_fd,file_fd,&offset,file_size);
        if(sent_data==-1){
            perror("Something went wrong while transmitting file");
            close(file_fd);
            return;
        }
        file_size-=sent_data;
    }
    close(file_fd);
}

//=====================
//      INTERFACE
//=====================

void handle_request(int socket_fd){
    FILE *conn_stream = fdopen(socket_fd,"r+");
    if(conn_stream==NULL) return;
    
    struct request_data* data;
    RESPONSE_CODE code = parse_request(conn_stream,&data);
    
    send_response(code,data,conn_stream,socket_fd);
    free_request_data(data);
    fclose(conn_stream);
}