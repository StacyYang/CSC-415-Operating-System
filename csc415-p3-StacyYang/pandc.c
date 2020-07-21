#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

//global variables
int* buffer;
int* producerArray;
int* consumerArray;
int producerIndex = 0;    //for producerArray
int consumerIndex = 0;    //for consumerArray
int insertIndex = 0;  //in
int removeIndex = 0;  //out
int shared_counter = 1;

sem_t full;     // keep track of the number of full spots 
sem_t empty;    // keep track of the number of empty spots 
pthread_mutex_t thread_lock;


int bufferNum;       //N
int producerNum;     //P
int consumerNum;     //C
int produceItemNum;  //X
int consumeItemNum;  //P*X/C
int pTime;           //Ptime
int cTime;           //Ctime

int overConsume = 0;  // set to 1 if over consume accurs
int overConsumeAmount = 0; // p*x/c + extra

void timestamp();  //print out current time
int grab_item();           //Function to remove items. Item removed is returned.
void put_item(int item);   //Function to put item into shared resource so it can be consumed.
void* producer(void* id);
void* consumer(void* id);
void* consumer1(void* id);
void check_match();


struct thread_info{
	pthread_t tid;
	int id; //readable id
};


int main(int argc, char* argv[]) {

    if(argc == 7){

        timestamp(); //print out current time before start

        //parse and print out commandline arguments
        bufferNum = atoi(argv[1]);              //N
        producerNum = atoi(argv[2]);            //P
        consumerNum = atoi(argv[3]);            //C
        produceItemNum = atoi(argv[4]);         //X
        consumeItemNum = (producerNum * produceItemNum) / consumerNum;   //   P*X/C
        pTime = atoi(argv[5]);                  //Ptime
        cTime = atoi(argv[6]);                  //Ctime

        if((producerNum * produceItemNum) % consumerNum == 0){
            overConsume = 0;
            overConsumeAmount = 0;
        }else{
            overConsume = 1;
            overConsumeAmount = (producerNum * produceItemNum) % consumerNum + consumeItemNum;   
        }

        printf("\t\t\t\t    Number of buffers   N:  %d\n", bufferNum);
        printf("\t\t\t   Number of Producer threads   P:  %d\n", producerNum);
        printf("\t\t\t   Number of Consumer threads   C:  %d\n", consumerNum);
        printf("    Number of items each producer thread will produce   X:  %d\n", produceItemNum);
        printf(" Number of items each consumer thread will consume   PX/C:  %d\n", consumeItemNum);
        printf("      Over consume accurs? (1 means yes, 0 means no)     :  %d\n", overConsume);
        printf("     Overconsume thread will consume items' number is    :  %d\n", overConsumeAmount);
        printf("\tTime each Producer thread sleeps (seconds) Ptime :  %d\n",  pTime);
        printf("\tTime each Consumer thread sleeps (seconds) Ctime :  %d\n\n",  cTime);

        //allocate memory
        buffer = malloc(bufferNum * sizeof(int));
        producerArray = malloc(produceItemNum * producerNum * sizeof(int));
        consumerArray = malloc(consumeItemNum * consumerNum * sizeof(int));

        //initialize semaphore and mutex
        if( sem_init(&full, 0, 0) ){          //full = 0
            perror("Error: failed to init full...\n");
            exit(-1);
        }
        if( sem_init(&empty, 0, bufferNum) ){  //empty = N   :to prevent overflow
            perror("Error: failed to init empty...\n");
            exit(-1);
        }
        pthread_mutex_t thread_lock = PTHREAD_MUTEX_INITIALIZER;


        time_t seconds;
        long nano_seconds;
        struct timespec start_time, end_time;
        // struct timespec {                                                                                     
        //    time_t   tv_sec;        /* seconds */                                                             
        //    long     tv_nsec;       /* nanoseconds */                                                         
        // };  

        clock_gettime(CLOCK_REALTIME, &start_time);   //time tracking starts here
                                                      //spawn off all threads
        int i, j;
        struct thread_info producerThreads[producerNum];
        struct thread_info consumerThreads[consumerNum];

        //create producer threads
        for(i = 0; i < producerNum; i++){
            producerThreads[i].id = i + 1;
            pthread_create(&producerThreads[i].tid, NULL, producer, (void*)&producerThreads[i].id);
        }

        //create consumer threads
        if(overConsume == 0){
            for(j = 0; j < consumerNum; j++){
                consumerThreads[j].id = j + 1;
                pthread_create(&consumerThreads[j].tid, NULL, consumer, (void*)&consumerThreads[j].id);
            }
        }else{
            for(j = 0; j < consumerNum - 1; j++){
                consumerThreads[j].id = j + 1;
                pthread_create(&consumerThreads[j].tid, NULL, consumer, (void*)&consumerThreads[j].id);
            }
            consumerThreads[consumerNum - 1].id = consumerNum;
            pthread_create(&consumerThreads[consumerNum - 1].tid, NULL, consumer1, (void*)&consumerThreads[consumerNum - 1].id);
        }
        

        //join producer threads
        for(i = 0; i < producerNum; i++){
            int retVal = pthread_join(producerThreads[i].tid, NULL);
            if(retVal < 0){
                perror("Error: Joining producer threads failed...\n");
                exit(-1);
            }else{
                printf("Producer thread joined:  %d\n", producerThreads[i].id);
            }
		}

        //join consumer threads
        for(j = 0; j < consumerNum; j++){
            int retVal = pthread_join(consumerThreads[j].tid, NULL);
            if(retVal < 0){
                perror("Error: Joining consumer threads failed...\n");
                exit(-1);
            }else{
                printf("Consumer thread joined:  %d\n", consumerThreads[j].id);
            }
		}
	
        clock_gettime(CLOCK_REALTIME, &end_time);     //time tracking stops here

        timestamp(); //print out current time after end

        //run your test strategy
        check_match();

        seconds = end_time.tv_sec - start_time.tv_sec;
        nano_seconds = end_time.tv_nsec - start_time.tv_nsec;

        if(end_time.tv_nsec < start_time.tv_nsec){
            seconds--;
            nano_seconds = nano_seconds + 1000000000L;
        }
        printf("\033[1;36m");
        printf("Total runtime: %ld.%09ld seconds\n", seconds, nano_seconds);
        printf("\033[0m");

        //clean up memory and thread
        free(buffer);
        free(producerArray);
        free(consumerArray);
        if(sem_destroy(&full)){
            perror("Error: failed to destroy full...\n");
            exit(-1);
        }
        if(sem_destroy(&empty)){
            perror("Error: failed to destroy empty...\n");
            exit(-1);
        }
        pthread_mutex_destroy(&thread_lock);

    }else{
        printf("Invalid use of commandline arguments\n");
        printf("Valid usage: ./pandc nb np nc x pTime cTime\n ");
        printf("   nb = number of buffers.\n");
        printf("   np = number of producers.\n");
        printf("   nc = number of consumers.\n");
        printf("    x = number of items produced by each producer thread.\n");
        printf("ptime = time spent busy waiting between produced items IN SECONDS.\n");
        printf("ctime = time spent busy waiting between consumed items IN SECONDS.\n\n");
    }
       
       return 0;

}


void timestamp(){

    static char time_buffer[40];
    const struct tm *tm;
    time_t now;

    now = time (NULL);
    tm = localtime(&now);

    strftime(time_buffer, 40, "%d %B %Y %I:%M:%S %p", tm);
    printf("\033[1;36m");
    printf("\nCurrent time:   %s\n\n", time_buffer);
    printf("\033[0m");
    return;
}

void put_item(int item){  

    buffer[insertIndex] = item; //insert the item into buffer
    insertIndex = (insertIndex + 1) % bufferNum; //adjust insertIndex position, increment and modulo for bounded buffer
    producerArray[producerIndex++] = item; //insert the item into producerArray
       
}

void* producer(void* id){

    int *ptr = (int*) id;
    int number = *ptr;

    int counter = 0;
    int newItem;

    while(counter < produceItemNum){ //counter < X
        //lock
        sem_wait(&empty); //wait until value of semaphore empty is greater than 0, decrement the value of semaphore s by 1
        pthread_mutex_lock(&thread_lock);

        newItem = shared_counter++;
        printf("\033[1;32m");
        printf("\t%-3d was produced by producer->  %d\n", newItem, number);
        printf("\033[0m");
        put_item(newItem);
        counter++;

        //unlock
        pthread_mutex_unlock(&thread_lock);
        sem_post(&full); //increment the value of semaphore full by 1, if there are 1 or more threads waiting, wake 1
        sleep(pTime);
    }
    pthread_exit(0);
}

int grab_item(){

    int consumedItem;
    consumedItem = buffer[removeIndex];
    buffer[removeIndex] = '\0';
    removeIndex = (removeIndex + 1 ) % bufferNum;
    consumerArray[consumerIndex++] = consumedItem;
    return consumedItem;
    
}

void* consumer(void* id){

    int *ptr = (int*) id;
    int number = *ptr;

    int counter = 0;
    int consumedItem;

    while(counter < consumeItemNum){  //counter < px/c
        //lock
        sem_wait(&full);
        pthread_mutex_lock(&thread_lock);

        consumedItem = grab_item();
        printf("\033[1;34m");
        printf("\t%-3d was consumed by consumer->  %d\n", consumedItem, number);
        printf("\033[0m");
        counter++;

        pthread_mutex_unlock(&thread_lock);
        sem_post(&empty); 
        sleep(cTime);
    }
    pthread_exit(0);
}

void* consumer1(void* id){

    int *ptr = (int*) id;
    int number = *ptr;

    int counter = 0;
    int consumedItem;

    while(counter < overConsumeAmount){  //counter < px/c + extra   (producerNum * produceItemNum) % consumerNum + consumeItemNum;
        //lock
        sem_wait(&full);
        pthread_mutex_lock(&thread_lock);

        consumedItem = grab_item();
        printf("\033[1;34m");
        printf("\t%-3d was consumed by consumer->  %d\n", consumedItem, number);
        printf("\033[0m");
        counter++;

        pthread_mutex_unlock(&thread_lock);
        sem_post(&empty); 
        sleep(cTime);
    }
    pthread_exit(0);
}

void check_match(){
    
    int producerArraySize = producerNum * produceItemNum;
    int consumerArraySize;
    if(overConsume == 0){
        consumerArraySize = consumerNum * consumeItemNum;
    }else{
        consumerArraySize = (producerNum * produceItemNum) % consumerNum + consumerNum * consumeItemNum;
    }
    
    printf("***************************************************\n");
    printf("Running test...\n");
    printf("Producer Array\t     |Consumer Array\n");
    for(int i = 0; i < producerArraySize; i++){
        printf("%-20d | %-20d\n", producerArray[i], consumerArray[i]);
    }
    
    //print arrays and compare them
    int flag = 1;
    if(producerArraySize != consumerArraySize){
        printf("\n Producer and consumer arrays Does Not match! Array size is different.\n");
        printf("\n Producer Array size is %d,  Consumer Array size is %d.\n\n", producerArraySize, consumerArraySize);
        return;
    }else{
        for(int i = 0; i < producerArraySize; i++){
            if(producerArray[i] != consumerArray[i]){
                flag = 0;
            }else{
                flag = 1;
            }
        }
    }

    if(flag == 0){
        printf("\n Producer and consumer arrays Does Not match!\n\n");
    }else{
        printf("\n Producer and consumer arrays match!\n\n");
    }
}