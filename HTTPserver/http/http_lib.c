#include "http_lib.h"
#include <errno.h>

__thread FILE* DATA_STREAM;
__thread int DATA_SOCKET; 

//=====================
//      AUXILIARY
//=====================

// This function changes the PATH to the requested file, to one that handles the error code
void handle_error_code(
    struct request_data* rq_data,
    RESPONSE_CODE code,
    struct server_config_data* server_info
){
    if(code==OK_200) return;

    // Add customization for error files
    free_path_data(rq_data->PATH_DATA);

    rq_data->PATH_DATA = init_path_data("/error.html",server_info); 
    
    // log_debug("New path for file:");
    // log_path_data(rq_data->PATH_DATA);
}

// Basic sanity check and uniforming for file names
char *adapt_filename(char* file,struct server_config_data* server_info){
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
    char *full_filename = malloc(sizeof(char)*(strlen(server_info->ROOT_FOLDER)+strlen(file)+strlen("index.html")+1));
    strcpy(full_filename,server_info->ROOT_FOLDER);
    strcat(full_filename,file);
    // if(file[strlen(file)-1]=='/') strcat(full_filename,"index.html");
    return full_filename;
}

METHOD str_to_method(char* method_str){
    if(method_str==NULL) return NONE;
    METHOD result = NONE;
    if(strcmp(method_str,"GET")==0) result = GET;
    if(strcmp(method_str,"HEAD")==0) result = HEAD;
    if(strcmp(method_str,"POST")==0) result = POST;
    return result;
}

// Check if the "If-Modified-Since" value is present in the request header
// If the field is specified, return the given string date, returns NULL otherwise
char* is_conditional_request(
    struct request_data* rq
){
    struct header_field* ptr = rq->other_fields;
    // Iterate over all fields and check for match
    while(ptr!=NULL){
        if(strcmp(ptr->NAME,"If-Modified-Since")==0) return ptr->VALUE;
        ptr = ptr->next_field;
    }   
    return NULL;
}

void flush_data_stream(){
    fseek(DATA_STREAM,0L,SEEK_CUR); 
}

void force_500();

//===========================
//
//   RESPONSE TRANSMISSION
//
//===========================

// Opens the file that needs to be sent and returns file descriptor
int open_response_file(char* filename){
    int file_fd = open(filename,O_RDONLY,NULL);
    if(file_fd==-1){
        log_debug("Something went wrong with open while sending response");
        force_500();
        return -1;      // unreachable
    }
    return file_fd;
}

// Sends the header of the response
void send_response_header(
    struct response_data rd_data
){
    // Send first header line
	char* first_line = resp_to_str(rd_data);
    fputs(first_line,DATA_STREAM);
	free(first_line);
	
	// Send rest of header
	struct header_field* aux = rd_data.other_fields;
	while(aux!=NULL){
		char* buffer = header_to_str(*aux);
		fputs(buffer,DATA_STREAM);
		free(buffer);
		aux = aux->next_field;
	}
}

// Sends the response body, getting data from the file descriptor file_fd, of size file_size
void send_response_body(int file_fd,off_t file_size){
    // Transmit file data
    off_t offset = 0;
    while(file_size!=0){
        int sent_data = sendfile(DATA_SOCKET,file_fd,&offset,file_size);
        if(sent_data==-1){
            perror("Something went wrong while transmitting file");
            close(file_fd);
            return;
        }
        file_size-=sent_data;
    }
}

// Send response to a request
void send_response_data
    (
        struct request_data* rq_data,
        char* filename,
        struct response_data rd_data,
        off_t file_size,
        bool file_send,
        struct server_config_data* server_info
    )
{
    if(filename==NULL && file_send){ force_500(); return; }

    int file_fd = (file_send ? open_response_file(filename): -1);

    send_response_header(rd_data);

    // Check if CGI
    if( 
        rq_data!=NULL 
        && rq_data->PATH_DATA->file_extension!=NULL 
        && strcmp(rq_data->PATH_DATA->file_extension,"php")==0 
        && file_send
    ){
        log_debug("Recognized as CGI script");
        flush_data_stream();

        prepare_cgi_env(rq_data,server_info);

        spawn_cgi_process(filename,file_fd,DATA_SOCKET);

        fputs("\r\n",DATA_STREAM);
        flush_data_stream();

        close(file_fd);
        return;
    }else{
        fputs("\r\n",DATA_STREAM);
        flush_data_stream();
    }

    if(!file_send) return;

    send_response_body(file_fd,file_size);
    close(file_fd);    
}

// This function should force a 500 response to the server in case of 
// bad failure
void force_500(){
    struct response_data* rd_data;
    log_critical("Forced 500 error, quitting application");
    rd_data = init_response("500","Internal Server Error","HTTP/1.0");
    send_response_data(NULL,NULL,*rd_data,0,false, NULL);
    // free_response_data(rd_data);
    fclose(DATA_STREAM);
    exit(-1);
}   


// Reads header's fields from the stream and add them to the request
bool parse_request_header_fields(
    struct request_data** rt
){
    char* line_buffer;
    char* saveptr;
    size_t line_size = 0;
    ssize_t read_val;

    for(;;){
        read_val = getline(&line_buffer,&line_size,DATA_STREAM);
        if(read_val==-1){ free(line_buffer); free(*rt); return false; }
        if(strcmp(line_buffer,"\r\n")==0) break;

        // TODO make this thread-safe
        char* NAME = strtok_r(line_buffer,":",&saveptr);
        char* VALUE = strtok_r(NULL,"\r\n",&saveptr);
        
        header_add_field_req(NAME,VALUE+1,(*rt));

        free(line_buffer);
        line_size = 0;
    }
    free(line_buffer);
    return true;
}

// This function parses the incoming request, returns a response code and the request data into rt
bool parse_request(
    struct request_data** rt,
    struct server_config_data* server_info
){
    if(DATA_STREAM==NULL) return false;

    *rt = NULL;
    char* line_buffer;
    ssize_t read_val;
    size_t line_size = 0;

    // Read the first header line
    {
        read_val = getline(&line_buffer,&line_size,DATA_STREAM);
        if(read_val==-1){ 
            free(line_buffer);
            perror("Something went wrong while receiving a request header");
            return false;
        }
        log_debug("Received request: %s",line_buffer);
        line_size=0;
    }

    // TODO change for thread safety
    char* saveptr;
    char* method = strtok_r(line_buffer," ",&saveptr);
    char* file = strtok_r(NULL," ",&saveptr);
    char* version = strtok_r(NULL,"\r\n",&saveptr);

    // Check for empty index request
    if(strcmp(file,"/")==0){
        file = "/index.html";
    }

    struct path_request_data* path_data = init_path_data(file,server_info); 

    // char* full_filename = adapt_filename(file);
    (*rt) = init_request(method,path_data,version);

    free(line_buffer);

    // This is the last action in the function, if it fails, the function should fail
    return parse_request_header_fields(rt);
}

void create_response
    (
        struct response_data** rd_data,
        struct request_data* rq_data,
        off_t* file_size,
        bool* file_send,
        struct server_config_data* server_info
    )
{
	if(rd_data==NULL || rq_data==NULL) return;

    METHOD md = str_to_method(rq_data->METHOD);
    RESPONSE_CODE ret_code = OK_200;
    *file_send = true;

    // Parse conditional date if present
    char* conditional_date = is_conditional_request(rq_data);

    if(conditional_date==NULL)
        log_debug("Request is not conditional");
    else
        log_debug("Request is conditional, with date: %s",conditional_date);

    switch(md){
        case HEAD:
            *file_send = false;
        case POST:
        case GET:
            // Change permission based on type of request
            if(access(rq_data->PATH_DATA->full_path,R_OK)==-1){
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

            // Check for conditional 
            if(conditional_date == NULL || md != GET) break;

            {

                struct stat file_data;
                if(stat(rq_data->PATH_DATA->real_full_path,&file_data)==-1){
                    perror("Something went wrong with STAT syscall\n");
                    force_500();	
                    return;
                }

                time_t last_modified = file_data.st_mtime;

                // Convert conditional time
                struct tm requested_tm; 
                memset(&requested_tm, 0, sizeof(struct tm));
                // Add compatibility with other format supported by the RFC
                if(strptime(conditional_date,"%a, %d %b %Y %H:%M:%S %Z",&requested_tm)==NULL){
                    ret_code = BAD_REQ_400;
                    break;
                }
                // TODO: check for invalid date (e.g. future date)
                // Convert tm to time_t
                time_t requested_time_t = mktime(&requested_tm);

                // Send file only if last modification date is newer than requested date
                if(last_modified<=requested_time_t){
                    *file_send = false;
                    ret_code = NOT_MOD_304;
                }
            }

            break;
        case NONE:
            ret_code = NOT_IMPLEMENTED_501;
            *file_send = false;
    }

    // TODO do better with macros
	switch(ret_code){
		case OK_200:
			*rd_data = init_response("200","OK",server_info->HTTP_VERSION);
			break;
        case NOT_MOD_304:
            *rd_data = init_response("304","Not Modified",server_info->HTTP_VERSION);
            break;
        case BAD_REQ_400:
            *rd_data = init_response("400","Bad Request",server_info->HTTP_VERSION);
            break;
		case FORBIDDEN_403: 
			*rd_data = init_response("403","Forbidden",server_info->HTTP_VERSION);
			break;
		case NOT_FOUND_404:
			*rd_data = init_response("404","Not Found",server_info->HTTP_VERSION);
			break;
        case NOT_IMPLEMENTED_501:
            *rd_data = init_response("501","Not Implemented",server_info->HTTP_VERSION);
			*file_send = false;
			break;
		default:
			*rd_data = init_response("500","Internal Server Error",server_info->HTTP_VERSION);
			break;
	}

    handle_error_code(rq_data,ret_code,server_info);

    header_add_field_resp("Connection","close",*rd_data);

	header_add_field_resp("Server",server_info->SERVER_ID,*rd_data);

	struct stat file_data;
	if(stat(rq_data->PATH_DATA->real_full_path,&file_data)==-1){
		perror("Something went wrong with STAT syscall\n");
		force_500();	
		return;
	}

    if(file_send) (*file_size) = file_data.st_size;

	// TODO can you do this better?
    
	#define BUFF_SIZE 500
	char aux[BUFF_SIZE];

	// sprintf(aux,"%ld",(long int) file_data.st_size);
	// header_add_field_resp("Content-Length",aux,*rd_data);

	// Get current date
	time_t* temp = malloc(sizeof(time_t));
	struct tm* curr_time;
	time(temp);
	curr_time = gmtime(temp);

	strftime(aux,BUFF_SIZE,"%a, %d %b %Y %H:%M:%S %Z",curr_time);
	free(temp);
	header_add_field_resp("Date",aux,*rd_data);

	// TODO Last modified
    
}

void send_response(struct request_data* rq_data,struct server_config_data* server_info){

	struct response_data* rd_data;
    off_t file_size;
    bool send_file;
	create_response(&rd_data,rq_data,&file_size,&send_file,server_info);
    log_info("Sending response");
	log_response(*rd_data);

    send_response_data(rq_data,rq_data->PATH_DATA->full_path,*rd_data,file_size,send_file,server_info);

    free_response_data(rd_data);
}

//=====================
//      INTERFACE
//=====================

void* handle_request(void* socket,struct server_config_data* server_info){
    DATA_SOCKET = *((int*) socket);
    DATA_STREAM = fdopen(DATA_SOCKET,"r+");
    if(DATA_STREAM==NULL) return NULL;
    
    struct request_data* data;
    bool success = parse_request(&data,server_info);

    log_request(*data);

    if(!success){
        force_500();
        return NULL;
    }

    send_response(data,server_info);
    free_request_data(data);

    fclose(DATA_STREAM);
    return NULL;
}