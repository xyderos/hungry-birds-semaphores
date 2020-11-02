#ifndef _REENTRANT
#define _REENTRANT
#endif
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>

#define NOF_KIDS 8

#define NOF_WORMS 5

//lock
sem_t mutex;

//queue
sem_t cond;

//the birds will be eating from this buffer(acts as a "bowl of worms")
int buffer[NOF_WORMS];

//producer variable used as index to fill the bowl
int front = 0;

//consumer variable used as index available to eat from the bowl
int rear = 0;

bool emptySpace = false;

void put(int value){

    buffer[front] = value;

    //update the value of front that the producer is using(acts like an index at the bowl,used module since it will always be within 0 and the number of birds)
    front = (front + 1) % NOF_WORMS;
}

int get(){

    int tmp = buffer[rear];

    //update the value oof rear that the consumers are using(acts like an index at the bowl, used modulo since it will always be within 0 and the number of birds)
    rear = (rear + 1) % NOF_WORMS;

    return tmp;
}

void *producer(void *arg){

    for(;;){

        //there are no worms remaining and father must fill the plate
        if(!emptySpace){

            //lock
            sem_wait(&mutex);

            //fill in the plate
            for(int i=0;i<NOF_WORMS; i++) put(i);

            printf("DAD FILLED THE PLATE\n");

            //plat is filled up again
            emptySpace=true;

            //signal the queue
            sem_post(&cond);

            //unlock
            sem_post(&mutex);
        }
    }
}

void *consumer(void *arg){

    long id = (long) arg;

    int temp, position;

    for(;;){

        //lock
        sem_wait(&mutex);

        //signal if there in no empty space and the plate needs to be filled with worms, signal and wait to be filled by father
        //(needs to unlock the mutex since the father will use that and we need to prevent deadlock)
        while(!emptySpace){

            //unlock
            sem_post(&mutex);

            //signal
            sem_wait(&cond);

            //reaquire again
            sem_wait(&mutex);
        }

        //get the next position
        //(other threads might finish the plate and ask for a refill so some birds might eat the same number of worms eg thread 0 ate number 2 and thread 3 ate number 2 as well)
        position = rear;

        //get the next position of available worn and eat it(the decrement is happening on the get function by changing the position of the next available worm)
        temp=get();

        printf("BABY NUMBER %ld ATE WORM WITH NUMBER %d\n", id, temp);

        //there is no empty space for the birds to eat so emptySpace will signal the father to refill the plate
        if(position==(NOF_WORMS-1)) emptySpace = false;

        //unlock
        sem_post(&mutex);

        //trying to avoid starvation to other threads since by eating and going to sleep they will be blocvked and the next thread in the queue will execute
        sleep(5);
    }
}

/**
the solution given is kind of unfair since if there is no sleep or a small sleep there will be a race condition whom takes the worms because of sem_wait
*/
int main(){

    pthread_t cons[NOF_KIDS];

    pthread_t prod;

    sem_init(&mutex, 0, 1);

    sem_init(&cond, 0, 0);

    pthread_create(&prod, NULL, producer, NULL);

    for(long i = 0; i<NOF_KIDS; i++) pthread_create(&cons[i], NULL, consumer, (void *)i);

    pthread_exit(NULL);
}
