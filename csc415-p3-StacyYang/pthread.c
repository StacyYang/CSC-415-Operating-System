#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_THREADS 30
#define MAX_ITERATIONS 30



/**
*pthread_create
*pthread_join
*pthread_exit
*pthread_t
*
*pthread_mutex_lock
*pthread_mutex_unlock
*PTHREAD_MUTEX_INITIALIZER
*pthread_mutex_t
*/

struct thread_info{
    pthread_t tid;
    int readable_id;
};

int shared_counter = 0;
pthread_mutex_t thread_lock = PTHREAD_MUTEX_INITIALIZER;
struct timespec ts = {0,999}; //nanosleep

void *adder(void* arg){
    struct thread_info* tinfo = (struct thread_info*)arg;
    int i, temp;
    for(i = 0; i < MAX_ITERATIONS; i++){

        pthread_mutex_lock(&thread_lock);
        temp = shared_counter;
        temp++;
        shared_counter = temp;
        pthread_mutex_unlock(&thread_lock);

        //printf("Adder %d wrote the valuse %d to shared counter\n", tinfo->readable_id, temp);
    }
    pthread_exit(tinfo->readable_id);
    //printf("Adder--> %3d\n", tinfo->readable_id);
}

void *subber(void* arg){
    struct thread_info* tinfo = (struct thread_info*)arg;
    int i, temp;
    for(i = 0; i < MAX_ITERATIONS; i++){

        pthread_mutex_lock(&thread_lock);
        temp = shared_counter;
        temp--;
        shared_counter = temp;
        pthread_mutex_unlock(&thread_lock);

        //printf("Subber %d wrote the valuse %d to shared counter\n", tinfo->readable_id, temp);
    }
    pthread_exit(tinfo->readable_id);
    //printf("subber--> %3d\n", tinfo->readable_id);
}


int main(int argc, char **argv){
    struct thread_info tinfo[MAX_THREADS];
    int i, j;

    for(i = 0; i < MAX_THREADS; i++){
        tinfo[i].readable_id = i;
        if(i % 2 == 0){
            pthread_create(&tinfo[i].tid, NULL, adder, (void *)&tinfo[i]);
        }else{
            pthread_create(&tinfo[i].tid, NULL, subber, (void *)&tinfo[i]);
        }
        
    }

    for(j = 0; j < MAX_THREADS; j++){
        pthread_join(tinfo[j].tid, NULL);
    }
    printf("All threads are terminated, shared value is: %d\n", shared_counter);

    return 0;
}