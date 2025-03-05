#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> /* for thread functions */
#include <errno.h> /* For the macros used here - EAGAIN, EINVAL, EPERM. etc.*/

/* Compile and link with -pthread, for example "gcc -pthread threadMutex1.c" */

/* Program to demonstrate mutex locks in thread programming */

#define nproducer 2 /* number of producers */
#define nconsumer 3 /* number of consumers */

struct passedData {
    int *data;
    int producer_no;
    int consumer_no;
};
struct passedData *threadData;

int sharedData = 0; /* the shared integer to be used by all threads */

/* Following functions (producer() and consumer()) will be executed by the threads.
   producer() will be executed by producer threads and
   consumer() will be executed by consumer threads */
/* This signatures of producer()  and consumer() are fixed as imposed by pthread_create().
   That is, they must take 'void *' as parameter and return 'void *' */
void *producer( void *param);
void *consumer( void *param);

/* Defining mutexe */
pthread_mutex_t pc_mutex; /* producer_consumer_mutex */

int main() {
    int i;
        pthread_t ptids[nproducer]; /* nproducer number of producer threads will be created, ptids[i] will store corresponding producer thread id */
        pthread_t ctids[nconsumer]; /* nconsumer number of consumer threads will be created, ctids[i] will store corresponding consumer thread id */
    int status; /* Used to store the return value (success/failure) of functions */

        pthread_attr_t attr; /*Set of thread attributes required to be passed in pthread_create() */
                 
    /* Initialize the mutex */
    //int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr);
    status = pthread_mutex_init(&pc_mutex, NULL);

    if (status != 0) { /* pthread_mutex_init() failed */
        /* Do not use perror since pthreads functions  do  not  set  errno. */
        // perror("pthread_mutex_init() Failed: "); Cannot use this here
       
        /* Consult pthread_mutex_init() manual for information on return value for failure */
        fprintf(stderr, "pthread_mutex_init() failed: %s.\n", status == EAGAIN?"The system lacked the necessary resources (other than memory) to initialize another mutex.":status == ENOMEM?"Insufficient memory exists to initialize the mutex.":status == EPERM?"The caller does not have the privilege to perform the operation.":status==EINVAL?"The attributes object referenced by attr has the robust mutex attribute set without the process-shared attribute being set.":"Unknown Error");
        exit(1);
    }

        /* Set the default attributes to be used by pthread_create() */
    /* int pthread_attr_init(pthread_attr_t *attr); */
        pthread_attr_init(&attr);  /* same attr will be used for creating all the threads */


    /* initialize the shared integer */
    sharedData = 0;

    /* create nproducer producer threads */
    for(i = 0; i < nproducer; i++) {

        threadData = (struct passedData *)malloc(sizeof(struct passedData));

        /* prepare the data to be passed to pthread_create() */
        threadData->data = &sharedData;
        /* set which producer thread it will be */
        threadData->producer_no = i;
             /* Now create the thread passing the threadData (shared data and producer thread no) as a parameter */
        /*int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg); */
             status = pthread_create(&ptids[i], &attr, producer, threadData);
        if (status != 0) { /* pthread_create() failed */
            /* Do not use perror since pthreads functions  do  not  set  errno. */
            // perror("pthread_create() Failed: "); Cannot use this here
           
            /* Consult pthread_create() manual for information on return value for failure */
            fprintf(stderr, "pthread_create() failed: %s.\n", status == EAGAIN?"Insufficient resources to create another thread OR A  system-imposed  limit on the number of threads was encountered.":status == EINVAL?"Invalid settings in attr.":status == EPERM?"No permission to set the scheduling policy and parameters specified in attr.":"Unknown Error");
            exit(1);
        }

    }
    /* create nconsumer producer threads */
    for(i = 0; i < nconsumer; i++) {
        threadData = (struct passedData *)malloc(sizeof(struct passedData));

        /* prepare the data to be passed to pthread_create() */
        threadData->data = &sharedData;
        /* set which consumer thread it will be */
        threadData->consumer_no = i;
             /* Now create the thread passing the threadData (shared data and consumer thread no) as a parameter */
        /*int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg); */
             status = pthread_create(&ctids[i], &attr, consumer, threadData);
        if (status != 0) { /* pthread_create() failed */
            /* Do not use perror since pthreads functions  do  not  set  errno. */
            // perror("pthread_create() Failed: "); Cannot use this here
           
            /* Consult pthread_create() manual for information on return value for failure */
            fprintf(stderr, "pthread_create() failed: %s.\n", status == EAGAIN?"Insufficient resources to create another thread OR A  system-imposed  limit on the number of threads was encountered.":status == EINVAL?"Invalid settings in attr.":status == EPERM?"No permission to set the scheduling policy and parameters specified in attr.":"Unknown Error");
            exit(1);
        }
    }
    /* wait for all producer and consumer threads */

        for(i = 0; i < nproducer; i++) {
        /* The parent thread waits for all the threads to complete */
        /* int pthread_join(pthread_t thread, void **retval) */
        status = pthread_join(ptids[i], NULL);
        if (status != 0) { /* pthread_join() failed */
            /* Do not use perror since pthreads functions  do  not  set  errno. */
            // perror("pthread_join() Failed: "); Cannot use this here

            /* Consult pthread_join() manual for information on return value for failure */
            fprintf(stderr, "pthread_join() failed: %s.\n", status == EDEADLK?"A deadlock was detected (e.g., two threads tried to join with each other); or thread specifies the calling thread.":status == EINVAL?"thread is not a joinable thread OR Another thread is already waiting to join with this thread.":status == ESRCH?"No thread with the ID thread could be found.":"Unknown Error");
            exit(1);
        }
    }
        for(i = 0; i < nconsumer; i++) {
        /* The parent thread waits for all the threads to complete */
        /* int pthread_join(pthread_t thread, void **retval) */
        status = pthread_join(ctids[i], NULL);
        if (status != 0) { /* pthread_join() failed */
            /* Do not use perror since pthreads functions  do  not  set  errno. */
            // perror("pthread_join() Failed: "); Cannot use this here

            /* Consult pthread_join() manual for information on return value for failure */
            fprintf(stderr, "pthread_join() failed: %s.\n", status == EDEADLK?"A deadlock was detected (e.g., two threads tried to join with each other); or thread specifies the calling thread.":status == EINVAL?"thread is not a joinable thread OR Another thread is already waiting to join with this thread.":status == ESRCH?"No thread with the ID thread could be found.":"Unknown Error");
            exit(1);
        }
    }

  return 0;
}

/* The producer() function is executed by all producer threads */
/* Please note that the signature (return type and parameter type) of such functions (the ones to be executed
   by a thread created by pthread_create() is fixed. That means all the "parameters" that a thread may need
   should be "grouped" and its address should be passed */
void *producer(void *param) {
    int status; /* used to store return value from pthread functions */
        struct passedData *alldata; /* contains the integer variable that holds data to be modified by the producer
                    and this producer no. */

    alldata = (struct passedData *) param;

    while(1) {
    fprintf(stderr, "I am producer thread [%d] got data = %d.\n", alldata->producer_no, *(alldata->data));

    /* Locking before modifying the shared data*/
    /* int pthread_mutex_lock(pthread_mutex_t *mutex); */
    status =  pthread_mutex_lock(&pc_mutex); /* check status for error */
    /* increment shared data */
    (*(alldata->data))++;
    fprintf(stderr, "I am producer thread [%d] incremented data to %d.\n", alldata->producer_no, *(alldata->data));
    /* unlocking after modifying the shared data*/
    /*int pthread_mutex_unlock(pthread_mutex_t *mutex); */
    status = pthread_mutex_unlock(&pc_mutex); /* check status for error */
    getchar(); /* for waiting user response */
    }


    /* Free the data malloced in the main thread */
    free(param);
  
        /* Exit the thread */
        pthread_exit(0);
} 

/* The consumer() function is executed by all consumer threads */
/* Please note that the signature (return type and parameter type) of such functions (the ones to be executed
   by a thread created by pthread_create() is fixed. That means all the "parameters" that a thread may need
   should be "grouped" and its address should be passed */
void *consumer(void *param) {
    int status; /* used to store return value from pthread functions */
        struct passedData *alldata; /* contains the integer variable that holds data to be read by the consumer
                    and this consumer no. */

    alldata = (struct passedData *) param;

    while(1) {
        fprintf(stderr, "I am consumer thread [%d] got data = %d.\n", alldata->consumer_no, *(alldata->data));
        getchar(); /* for waiting user response */
    }

    /* Free the data malloced in the main thread */
    free(param);
  
        /* Exit the thread */
        pthread_exit(0);
}  