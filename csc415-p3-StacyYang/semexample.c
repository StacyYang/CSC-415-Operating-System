#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/types.h>


#define MAX_THREADS 500
#define MAX_CONNECTIONS 10
#define MAX_PROCESS_TIME 5

/**
* sem_init
* sem_open (for mac)
* sem_post
* sem_wait
* sem_destroy
*/

/*   shared data   */
int total_request_count;
struct connection_pool *conns_pool;
sem_t binary_semaphore;
sem_t connection_count_semaphore;
/*    end of shared data*/


struct thread_info{
    pthread_t tid;
    int id;
};

struct connection{
    int id;
    int owner_id;
    int process_time;
    int request_count;
    int total_process_time;
};

struct connection_pool{
    struct connection *cons;
    int in;
    int out;
};

int rand_process_time(){
    return rand() % MAX_PROCESS_TIME;
}

struct connection get_connection(){
    int temp_out = conns_pool -> out;
    struct connection temp = conns_pool -> cons[temp_out];
    temp_out = (temp_out +1 ) % MAX_CONNECTIONS;
    conns_pool -> out = temp_out;
    return temp;
}

void put_back_connection(struct connection con){
    int temp_in;
    con.request_count++;
    con.total_process_time += con.process_time;
    con.owner_id = -1;
    con.process_time = -1;
    temp_in = conns_pool -> in;
    conns_pool -> cons[temp_in] = con;
    temp_in = (temp_in +1) % MAX_CONNECTIONS;
    conns_pool -> in = temp_in;
    total_request_count++;
}

void *process_request(void *arg){
    struct thread_info *tinfo = (struct thread_info *)arg;
    struct connection current_con;

    //can we get a connection
    sem_wait(&connection_count_semaphore);
    //lock the binary
    sem_wait(&binary_semaphore);
    //grab connection
    current_con = get_connection();
    //unlock the binary sema
    sem_post(&binary_semaphore);
    current_con.process_time = rand_process_time();
    printf("Thread: %3d is processing a request for %3d second(s) on connection %3d\n",
    tinfo -> id, current_con.process_time, current_con.id);
    //sleep() fake processsing a request
    sleep(current_con.process_time);
    //lock the binary
    sem_wait(&binary_semaphore);
    //put back the connection
    put_back_connection(current_con);
    //unlock the binary sema
    sem_post(&binary_semaphore);
    //signal count sema
    sem_post(&connection_count_semaphore);

}



int main(int argc, char **argv){
    struct thread_info tinfo[MAX_THREADS];
    int i, j;
    if(sem_init(&binary_semaphore, 0, 1)){
        perror("Failed to init binary semaphore.");
        exit(-1);
    }

    if(sem_init(&connection_count_semaphore, 0, MAX_CONNECTIONS)){  
        perror("Failed to init connection count semaphore.");
        exit(-1);
    }

    conns_pool = malloc(sizeof(struct connection_pool));
    if(conns_pool == NULL){
        perror("Failed to allocated connection pool");
        exit(-1);
    }

    conns_pool -> cons = calloc(sizeof(struct connection), MAX_CONNECTIONS);
    conns_pool -> in = 0;
    conns_pool -> out = 0;
    for(i = 0; i < MAX_CONNECTIONS;  i++){
        conns_pool -> cons[i].id = i + 1;
        conns_pool -> cons[i].request_count = 0;
        conns_pool -> cons[i].total_process_time = 0;
    }

    for(i = 0; i < MAX_THREADS; i++){
        tinfo[i].id = i;
        pthread_create(&tinfo[i].tid, NULL, process_request, (void *)&tinfo[i]);
    }

    for(j = 0; j < MAX_THREADS; j++){
        if(pthread_join(tinfo[j].tid, NULL)){
            perror("error joining thread");
            exit(-1);
        }
    }
    printf("%5d request were handled\n", total_request_count);
    printf("\n\nPer thread info: \n\n");
    int total_requests = 0;
    for(i = 0; i < MAX_CONNECTIONS; i++){
        struct connection current_con = conns_pool -> cons[i];
        printf("Connection: %3d handled %3d requests for a total process time of %3d\n", 
        current_con.id,
        current_con.request_count,
        current_con.total_process_time);
        total_requests += current_con.request_count;
    }
    printf("Total requests: %d\n", total_requests);
    if(sem_destroy(&binary_semaphore)){
        perror("Failed to destroy bin semaphore");
        exit(-1);
    }

    if(sem_destroy(&connection_count_semaphore)){
        perror("Failed to destroy bin semaphore");
        exit(-1);
    }

    return 0;
}
