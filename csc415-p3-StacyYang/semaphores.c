#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_THREADS 30
#define MAX_ITERATIONS 30
#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif

/**
* Linux             MacOS
* sem_init          dispatch_semaphore_create(value)
* sem_wait          dispatch_semaphore_wait()
* sem_post          dispatch_semaphore_post()
* sem_destroy
*/
struct linapple_semaphore{
    #ifdef __APPLE__
        dispatch_semaphore_t sem;
    #else
        sem_t sem;
    #endif
};

struct thread_info{
    pthread_t tid;
    int readable_id;
};

static inline void linapple_sem_init(struct linapple_semaphore* s, uint32_t value){
    #ifdef __APPLE__
        dispatch_semaphore_t* tsem = &s->sem;
        *tsem = dispatch_semaphore_create(value);
    #else
        sem_init(&s->sem, 0, value);
    #endif
}

static inline void linapple_sem_wait(struct linapple_semaphore* s){
    #ifdef __APPLE__
        dispatch_semaphore_wait(s->sem, DISPATCH_TIME_FOREVER);
    #else
        sem_wait(&s->sem);
    #endif
}

static inline void linapple_sem_post(struct linapple_semaphore* s){
    #ifdef __APPLE__
        dispatch_semaphore_signal(s->sem);
    #else
        sem_post(&s->sem);
    #endif
}

int shared_counter = 0;
// pthread_mutex_t thread_lock = PTHREAD_MUTEX_INITIALIZER;
struct linapple_semaphore shared_semaphore;
struct timespec ts = {0,999}; //nanosleep

void *adder(void* arg){
    struct thread_info* tinfo = (struct thread_info*)arg;
    int i, temp;
    for(i = 0; i < MAX_ITERATIONS; i++){

        // pthread_mutex_lock(&thread_lock);
        linapple_sem_wait(&shared_semaphore);
        temp = shared_counter;
        temp++;
        shared_counter = temp;
        linapple_sem_post(&shared_semaphore);
        // pthread_mutex_unlock(&thread_lock);

        //printf("Adder %d wrote the valuse %d to shared counter\n", tinfo->readable_id, temp);
    }
    pthread_exit(tinfo->readable_id);
    //printf("Adder--> %3d\n", tinfo->readable_id);
}

void *subber(void* arg){
    struct thread_info* tinfo = (struct thread_info*)arg;
    int i, temp;
    for(i = 0; i < MAX_ITERATIONS; i++){

        // pthread_mutex_lock(&thread_lock);
        linapple_sem_wait(&shared_semaphore);
        temp = shared_counter;
        temp--;
        shared_counter = temp;
        linapple_sem_post(&shared_semaphore);
        // pthread_mutex_unlock(&thread_lock);

        //printf("Subber %d wrote the valuse %d to shared counter\n", tinfo->readable_id, temp);
    }
    pthread_exit(tinfo->readable_id);
    //printf("subber--> %3d\n", tinfo->readable_id);
}


int main(int argc, char **argv){
    struct thread_info tinfo[MAX_THREADS];
    int i, j;
    linapple_sem_init(&shared_semaphore, 1);

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