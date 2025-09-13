#include "multithreading.h"

pthread_mutex_t thread_pool_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t thread_list[THREAD_POOL_SIZE];
bool available_threads[THREAD_POOL_SIZE] = {[0 ... THREAD_POOL_SIZE-1] = true };

bool thread_is_available(){
    pthread_mutex_lock(&thread_pool_mutex);
    for(int i=0;i<THREAD_POOL_SIZE;i++){
        if(available_threads[i]){
            pthread_mutex_unlock(&thread_pool_mutex);
            return true;
        }
    }
    pthread_mutex_unlock(&thread_pool_mutex);
    return false;
}

int num_available_threads(){
    int c = 0;
    for(int i=0;i<THREAD_POOL_SIZE;i++){
        if(available_threads[i]) c++;
    }
    return c;
}

void join_done_threads(){
    // TODO: check if this is right
    pthread_mutex_lock(&thread_pool_mutex);
    void* ptr;
    for(int i=0;i<THREAD_POOL_SIZE;i++){
        if(!available_threads[i]){
            if(pthread_tryjoin_np(thread_list[i],&ptr)==0) available_threads[i] = true;
        }
    }
    pthread_mutex_unlock(&thread_pool_mutex);
}

int request_thread(pthread_t** assigned_thread){
    pthread_mutex_lock(&thread_pool_mutex);
    for(int i=0;i<THREAD_POOL_SIZE;i++){
        if(available_threads[i]){
            available_threads[i]=false;
            *assigned_thread = &thread_list[i];
            pthread_mutex_unlock(&thread_pool_mutex);
            return i;
        }
    }
    pthread_mutex_unlock(&thread_pool_mutex);
    return -1;
}