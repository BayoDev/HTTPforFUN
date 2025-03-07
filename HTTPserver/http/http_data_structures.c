#include "http_data_structures.h"

//=====================
//   DATA STRUCTURES
//=====================

struct server_config_data* init_server_config(
    char* ROOT_FOLDER,
    char* PORT,
    char* HTTP_VERSION,
    char* SERVER_ID
){
    // Check for missing parameters
    if(ROOT_FOLDER==NULL || PORT==NULL || HTTP_VERSION==NULL || SERVER_ID==NULL) return NULL;

    struct server_config_data* new = malloc(sizeof(struct server_config_data));

    new->ROOT_FOLDER = malloc(sizeof(char)*(strlen(ROOT_FOLDER)+1));
    strcpy(new->ROOT_FOLDER,ROOT_FOLDER);

    // Calculate real path of root folder
    char* cwd = getcwd(NULL,0);
    new->ROOT_FOLDER_REAL_PATH = malloc(sizeof(char)*(strlen(cwd)+strlen(ROOT_FOLDER)+2));
    strcpy(new->ROOT_FOLDER_REAL_PATH,cwd);
    strcat(new->ROOT_FOLDER_REAL_PATH,"/");
    strcat(new->ROOT_FOLDER_REAL_PATH,ROOT_FOLDER);
    free(cwd);

    new->PORT = malloc(sizeof(char)*(strlen(PORT)+1));
    strcpy(new->PORT,PORT);

    new->HTTP_VERSION = malloc(sizeof(char)*(strlen(HTTP_VERSION)+1));
    strcpy(new->HTTP_VERSION,HTTP_VERSION);

    new->SERVER_ID = malloc(sizeof(char)*(strlen(SERVER_ID)+1));
    strcpy(new->SERVER_ID,SERVER_ID);

    return new;
}

struct request_data* init_request(char* METHOD,struct path_request_data* PATH_DATA,char* VERSION){
    if(METHOD==NULL || PATH_DATA == NULL || VERSION==NULL) return NULL;
    struct request_data* new = malloc(sizeof(struct request_data));

    new->METHOD = malloc(sizeof(char)*(strlen(METHOD)+1));
    strcpy(new->METHOD,METHOD);

    new->PATH_DATA = PATH_DATA;

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

struct path_request_data* init_path_data(char* request_path,struct server_config_data* server_info){

    struct path_request_data* new = malloc(sizeof(struct path_request_data));
    new->full_path = adapt_filename(request_path,server_info);

    new->requested_path = malloc(sizeof(char)*(strlen(request_path)+1));
    strcpy(new->requested_path,request_path);

    new->parameters = strchr(new->requested_path,'?');
    if( new->parameters == NULL) {
        new->parameters = malloc(sizeof(char));
        new->parameters = "";
    }
    else{
        new->parameters++;
    }

    new->filename = malloc(sizeof(char)*(strlen(new->requested_path)+1));
    strcpy(new->filename,new->requested_path);
    int idx = strlen(new->requested_path)-strlen(new->parameters);
    if(strcmp(new->parameters,"")!=0) idx--;
    new->filename[idx]='\0';

    new->real_full_path = malloc(sizeof(char)*(strlen(server_info->ROOT_FOLDER_REAL_PATH)+strlen(new->filename)+1));
    strcpy(new->real_full_path,server_info->ROOT_FOLDER_REAL_PATH);
    strcat(new->real_full_path,new->filename);

    new->file_extension = strrchr(new->full_path,'.');
    if(new->file_extension!=NULL) new->file_extension++;

    // new->local_path = malloc(sizeof(char)*(strlen(new->requested_path)+1));
    // strcpy(new->local_path,new->requested_path); 
    // new->local_path[strlen(new->requested_path)-strlen(new->filename)-strlen(new->parameters)-1]='\0';

    return new;
}

void log_request(struct request_data rq){
    log_debug("REQUEST DATA:");
    log_debug("\t%s %s",rq.METHOD,rq.VERSION);
    log_path_data(rq.PATH_DATA);
    struct header_field* aux = rq.other_fields;
    while(aux!=NULL){
        log_debug("\t%s: %s",aux->NAME,aux->VALUE);
        aux=aux->next_field;
    }
}

void log_path_data(struct path_request_data* pr_data){
    log_debug("\tPATH DATA:");
    log_debug("\t\tFULL PATH: %s",pr_data->full_path);
    log_debug("\t\tREAL FULL PATH: %s",pr_data->real_full_path);
    log_debug("\t\tREQUESTED PATH: %s",pr_data->requested_path);
    log_debug("\t\tFILENAME: %s",pr_data->filename);
    log_debug("\t\tPARAMETERS: %s",pr_data->parameters);
    log_debug("\t\tFILE EXTENSION: %s",pr_data->file_extension);
    // printf("\tLOCAL PATH: %s\n",pr_data->local_path);
}

void log_response(struct response_data rd){
    log_debug("RESPONSE DATA:");
    log_debug("\t%s %s %s",rd.VERSION,rd.CODE,rd.DESC);
    struct header_field* aux = rd.other_fields;
    while(aux!=NULL){
        log_debug("\t\t%s: %s",aux->NAME,aux->VALUE);
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

void free_path_data(struct path_request_data* pr_data){
    free(pr_data->full_path);
    free(pr_data->requested_path);
    free(pr_data->filename);
    free(pr_data->real_full_path);
    free(pr_data);
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
    free(st->METHOD);
    free_path_data(st->PATH_DATA);
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