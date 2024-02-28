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

typedef enum{
    GET,
    HEAD,
    POST,
    NONE        // Not implemented
} METHOD;

__thread FILE* DATA_STREAM;
__thread int DATA_SOCKET; 

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

METHOD str_to_method(char* method_str){
    if(method_str==NULL) return NONE;
    METHOD result = NONE;
    if(strcmp(method_str,"GET")==0) result = GET;
    if(strcmp(method_str,"HEAD")==0) result = HEAD;
    if(strcmp(method_str,"POST")==0) result = POST;
    return result;
}

void force_500();

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

struct response_data{
    char* CODE;
    char* DESC;
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

struct response_data* init_response(char* CODE,char* DESC,char* VERSION){
    if(CODE==NULL || DESC==NULL || VERSION==NULL) return NULL;
    struct response_data* new = malloc(sizeof(struct response_data));

    new->CODE = malloc(sizeof(char)*(strlen(CODE)+1));
    strcpy(new->CODE,CODE);
    new->DESC = malloc(sizeof(char)*(strlen(DESC)+1));
    strcpy(new->DESC,DESC);
    new->VERSION = malloc(sizeof(char)*(strlen(VERSION)+1));
    strcpy(new->VERSION,VERSION); 

    new->other_fields = NULL;
    return new;
}

void update_response(char* CODE,char* DESC,char* VERSION,struct response_data* rd_data){
	if(CODE!=NULL){
		rd_data->CODE = realloc(rd_data->CODE,sizeof(char)*(strlen(CODE)+1));
		strcpy(rd_data->CODE,CODE);
	}
	if(DESC!=NULL){
		rd_data->DESC = realloc(rd_data->DESC,sizeof(char)*(strlen(DESC)+1));
		strcpy(rd_data->DESC,DESC);
	}
	if(VERSION!=NULL){
		rd_data->VERSION = realloc(rd_data->VERSION,sizeof(char)*(strlen(VERSION)+1));
		strcpy(rd_data->VERSION,VERSION);
	}
}

void header_add_field_req(char* NAME,char* VALUE,struct request_data* st){
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

void header_add_field_resp(char* NAME,char* VALUE,struct response_data* st){
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
        printf("%s: %s\n",aux->NAME,aux->VALUE);
        aux=aux->next_field;
    }
}

void print_response(struct response_data rd){
    printf("%s %s %s\n",rd.VERSION,rd.CODE,rd.DESC);
    struct header_field* aux = rd.other_fields;
    while(aux!=NULL){
        printf("%s: %s\n",aux->NAME,aux->VALUE);
        aux=aux->next_field;
    }
}

char* resp_to_str(struct response_data rd){
	char* ret = calloc(strlen(rd.CODE)+strlen(rd.DESC)+strlen(rd.VERSION)+5,sizeof(char));
	sprintf(ret,"%s %s %s\r\n",rd.VERSION,rd.CODE,rd.DESC);
	return ret;
}

char* header_to_str(struct header_field rd){
	char* ret = calloc(strlen(rd.NAME)+strlen(rd.VALUE)+5,sizeof(char));
	sprintf(ret,"%s: %s\r\n",rd.NAME,rd.VALUE);
	return ret;
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

void free_response_data(struct response_data* st){
    if(st->other_fields!=NULL){
        free_header_field(st->other_fields);
        free(st->other_fields->NAME);
        free(st->other_fields->VALUE);
        free(st->other_fields);
    }
    free(st->CODE);
    free(st->DESC);
    free(st->VERSION);
    free(st);
}

//=====================
//     HANDLE I/O
//=====================

void send_data
    (
        char* filename,
        struct response_data rd_data,
        off_t file_size,
        bool file_send
    )
{
    if(filename==NULL){ force_500(); return; }

    int file_fd;
    if(file_send){
        // Open file that needs to be sent
        file_fd = open(filename,O_RDONLY,NULL);
        if(file_fd==-1){
            debug("Something went wrong with open while sending response\n");
            force_500();
            return;
        }
    }

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

	fputs("\r\n",DATA_STREAM);
    fseek(DATA_STREAM,0L,SEEK_CUR);                         // sync

    if(!file_send) return;

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
    close(file_fd);
}

// This function should force a 500 response to the server in case of 
// bad failure
void force_500(){
    struct response_data* rd_data;
    rd_data = init_response("500","Internal Server Error","HTTP/1.0");
    send_data(NULL,*rd_data,0,false);
    free_response_data(rd_data);
    fclose(DATA_STREAM);
    exit(-1);
}   

// This function return parse the request, returns a response code and the request data into rt
bool parse_request(struct request_data** rt){
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
            perror("Something went wrong while receiving a request header\n");
            return false;
        }
        debug("[DEBUG] Received request: %s",line_buffer);
        line_size=0;
    }

    // TODO change for thread safety
    char* saveptr;
    char* method = strtok_r(line_buffer," ",&saveptr);
    char* file = strtok_r(NULL," ",&saveptr);
    char* version = strtok_r(NULL,"\r\n",&saveptr);

    char* full_filename = adapt_filename(file);
    (*rt) = init_request(method,full_filename,version);

    free(full_filename);
    free(line_buffer);

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

void create_response
    (
        struct response_data** rd_data,
        struct request_data* rq_data,
        off_t* file_size,
        bool* file_send
    )
{
	if(*rd_data==NULL || rq_data==NULL) return;

    METHOD md = str_to_method(rq_data->METHOD);
    RESPONSE_CODE ret_code = OK_200;
    *file_send = true;

    switch(md){
        case HEAD:
            *file_send = false;
        case GET:
            // Change permission based on type of request
            if(access(rq_data->FILE,R_OK)==-1){
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
            break;
        case POST:
        case NONE:
            ret_code = NOT_IMPLEMENTED_501;
            *file_send = false;
    }

	switch(ret_code){
		case OK_200:
			*rd_data = init_response("200","OK","HTTP/1.0");
			break;
		case FORBIDDEN_403: 
			*rd_data = init_response("403","Forbidden","HTTP/1.0");
			free(rq_data->FILE);
			rq_data->FILE = adapt_filename(FILE_403);
			break;
		case NOT_FOUND_404:
			*rd_data = init_response("404","Not Found","HTTP/1.0");
			free(rq_data->FILE);
			rq_data->FILE = adapt_filename(FILE_404);
			break;
        case NOT_IMPLEMENTED_501:
            *rd_data = init_response("501","Not Implemented","HTTP/1.0");
			file_send = false;
			break;
		default:
			*rd_data = init_response("500","Internal Server Error","HTTP/1.0");
			free(rq_data->FILE);
			rq_data->FILE = adapt_filename(FILE_500);
			break;
	}

    header_add_field_resp("Connection","close",*rd_data);

	header_add_field_resp("Server",SERVER_ID,*rd_data);

	struct stat file_data;
	if(stat(rq_data->FILE,&file_data)==-1){
		perror("Something went wrong with STAT syscall\n");
		force_500();	
		return;
	}

    (*file_size) = file_data.st_size;

	// TODO can you do this better?
	#define BUFF_SIZE 500

	char aux[BUFF_SIZE];
	sprintf(aux,"%ld",(long int) file_data.st_size);
	header_add_field_resp("Content-Length",aux,*rd_data);

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

void send_response(struct request_data* rq_data){

	struct response_data* rd_data;
    off_t file_size;
    bool send_file;
	create_response(&rd_data,rq_data,&file_size,&send_file);
    debug("\n\n[DEBUG] Sending response\n");
	print_response(*rd_data);

    send_data(rq_data->FILE,*rd_data,file_size,send_file);

    free_response_data(rd_data);
}

//=====================
//      INTERFACE
//=====================

void* handle_request(void* socket){
    DATA_SOCKET = *((int*) socket);
    DATA_STREAM = fdopen(DATA_SOCKET,"r+");
    if(DATA_STREAM==NULL) return NULL;
    
    struct request_data* data;
    bool success = parse_request(&data);

    #ifdef DEBUG
    print_request(*data);
    #endif

    if(!success){
        force_500();
        return NULL;
    }

    send_response(data);
    free_request_data(data);

    fclose(DATA_STREAM);
    return NULL;
}