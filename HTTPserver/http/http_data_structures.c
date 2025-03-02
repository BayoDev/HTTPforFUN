#include "http_data_structures.h"

//=====================
//   DATA STRUCTURES
//=====================

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