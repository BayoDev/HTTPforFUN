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

void fail_request(char* str,FILE* conn_stream,int socket);

char *adapt_filename(char* file){
    char *full_filename = malloc(sizeof(char)*(strlen(ROOT_FOLDER)+strlen(file)+strlen("index.html")+1));
    strcpy(full_filename,ROOT_FOLDER);
    strcat(full_filename,file);
    if(file[strlen(file)-1]=='/') strcat(full_filename,"index.html");
    return full_filename;
}

void send_response(RESPONSE_CODE code,char* filename,FILE* conn_stream, int socket_fd){

    char HEADER_CONTENT[500];
    switch(code){
        case OK_200:
            strcpy(HEADER_CONTENT,"HTTP/1.0 200 OK\r\n\r\n");
            break;
        case FORBIDDEN_403: 
            strcpy(HEADER_CONTENT,"HTTP/1.0 403 Forbidden\r\n\r\n");
            break;
        case NOT_FOUND_404:
            strcpy(HEADER_CONTENT,"HTTP/1.0 404 Not Found\r\n\r\n");
            break;
        default:
            strcpy(HEADER_CONTENT,"HTTP/1.0 500 Internal Server error\r\n\r\n");
            break;
    }

    debug("[DEBUG] Sending file: '%s' with header: %s\n",filename,HEADER_CONTENT);

    // Send file
    int file_fd = open(filename,O_RDONLY,NULL);
    if(file_fd==-1){debug("Something went wrong with open while sending response"); return;} // TODO something else in case the file wont open

    struct stat file_stat;
    if(fstat(file_fd,&file_stat)==-1){debug("Something went wrong in 'stat' call while sending response");close(file_fd);return;}

    fputs(HEADER_CONTENT,conn_stream);
    fseek(conn_stream,0L,SEEK_CUR);                         // sync

    // TODO do proper send loop
    off_t file_size = file_stat.st_size;
    int sent_data = sendfile(socket_fd,file_fd,NULL,file_size);
    if(sent_data==-1) perror("Error occured in sendfile\n");
    close(file_fd);
}

void fail_request(char* str,FILE* conn_stream,int socket){
    perror(str);
    send_response(INTERNAL_ERROR_500,adapt_filename(FILE_500),conn_stream,socket);
}

void handle_request(int socket_fd){
    FILE *conn_stream = fdopen(socket_fd,"r+");
    if(conn_stream==NULL) return;
    char* line_buffer;
    ssize_t read_val;
    size_t line_size = 0;
    // Read the first header line
    {
        read_val = getline(&line_buffer,&line_size,conn_stream);
        if(read_val==-1){ free(line_buffer); fail_request("Something went wrong while receiving a request header\n",conn_stream,socket_fd); return;}
        debug("[DEBUG] Received request: %s",line_buffer);
        line_size=0;
    }

    // TODO change for thread safety
    char* method = strtok(line_buffer," ");
    char* file = strtok(NULL," ");

    char* full_filename = adapt_filename(file);

    free(line_buffer);

    // Read the remaining header
    for(;;){
        read_val = getline(&line_buffer,&line_size,conn_stream);
        if(read_val==-1){ free(line_buffer); break; }
        debug("\t%s",line_buffer);
        if(strcmp(line_buffer,"\r\n")==0) break;
        free(line_buffer);
        line_size = 0;
    }

    RESPONSE_CODE code = OK_200;

    debug("[DEBUG] File requested: %s \n",full_filename);
    // Get file data
    struct stat file_stat;
    if(stat(full_filename,&file_stat)==-1){
        switch(errno){
            case ENOENT:
                code = NOT_FOUND_404;
                free(full_filename);
                full_filename = adapt_filename(FILE_404);
                break;
            case EACCES:
                code = FORBIDDEN_403;
                free(full_filename);
                full_filename = adapt_filename(FILE_403);
                break;
            default:
                code = INTERNAL_ERROR_500;
                free(full_filename);
                full_filename = adapt_filename(FILE_500);
        }
    }

    send_response(code,full_filename,conn_stream,socket_fd);
    free(full_filename);
    fclose(conn_stream);
}