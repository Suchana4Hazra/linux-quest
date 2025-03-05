#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> /* for thread functions */
#include <errno.h> /* For the macros used here - EAGAIN, EINVAL, EPERM. etc.*/

/* Compile and link with -pthread, for example "gcc -pthread matrixmult1.c" */



/* A is an m X n matrix  and B is an n X r matrix and C is an m X r matrix */
#define m 2
#define n 3
#define r 2

int A[m][n]= {{1,2,3}, {4,5,6}};/* global array to store matrix A */
int B[n][r]= {{1,2}, {3,4}, {5,6}};/* global array to store matrix B */
int C[m][r]; /* global array to store matrix C */
 
/* Each thread will compute the C[i][j] element of the product */

/* The following structure will be used to pass i and j to a thread
   so that the thread knows which element to compute */
struct v {
    int i;
    int j;
    };

/* Following function (runner) will be executed by each thread */
/* This signature of runner() is fixed as imposed by pthread_create().
   That is, runner() must take 'void *' as parameter and return 'void *' */
void *runner( void *param);
  
int main() {
    int i, j;
    int status; /* Used to store the return value (success/failure) of functions */
                 
    /* create one thread for each of C[m][r] matrix */
    for(i = 0; i < m; i++) {
        for(j = 0; j < r; j++) {
                 pthread_t tid;       //Thread ID
                 struct v *data; /* each thread will be passed with i and j in 'struct v' */
                 pthread_attr_t attr; /*Set of thread attributes required to be passed in pthread_create() */
     
            /* Allocate a structure where values of i and j is put for passing it to the thread */
            data = (struct v *) malloc(sizeof(struct v));
            /* Question: why not define 'struct v data' and pass the address of the structure (&data)
               in place of 'struct v *data' and mallocing the structure to it as is done here!! */

                 /* Assign a row and column for each thread */
                 data->i = i;
                 data->j = j;

                 /* Set the default attributes */
            /* int pthread_attr_init(pthread_attr_t *attr); */
                 pthread_attr_init(&attr);
            /* The above is a wrong usage of pthread_attr_init(). Note that this function will be called
               m*r times on the same 'attr', whereas the manual suggests that -
               'Calling pthread_attr_init() on a thread attributes object that has already been initialized results in undefined behavior.'
               Correct this by moving it up outside any loop! :-P
            */

                 /* Now create the thread passing its data as a parameter */
            /*int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg); */
                 status = pthread_create(&tid, &attr, runner, data);
            if (status != 0) { /* pthread_create() failed */
                /* Do not use perror since pthreads functions  do  not  set  errno. */
                // perror("pthread_create() Failed: "); Cannot use this here
               
                /* Consult pthread_create() manual for information on return value for failure */
                fprintf(stderr, "pthread_create() failed: %s.\n", status == EAGAIN?"Insufficient resources to create another thread OR A  system-imposed  limit on the number of threads was encountered.":status == EINVAL?"Invalid settings in attr.":status == EPERM?"No permission to set the scheduling policy and parameters specified in attr.":"Unknown Error");
                exit(1);
            }

                 /* The parent thread waits for this thread to complete */
            /* int pthread_join(pthread_t thread, void **retval) */
                 status = pthread_join(tid, NULL);
            if (status != 0) { /* pthread_join() failed */
                /* Do not use perror since pthreads functions  do  not  set  errno. */
                // perror("pthread_join() Failed: "); Cannot use this here

                /* Consult pthread_join() manual for information on return value for failure */
                fprintf(stderr, "pthread_join() failed: %s.\n", status == EDEADLK?"A deadlock was detected (e.g., two threads tried to join with each other); or thread specifies the calling thread.":status == EINVAL?"thread is not a joinable thread OR Another thread is already waiting to join with this thread.":status == ESRCH?"No thread with the ID thread could be found.":"Unknown Error");
                exit(1);
            }

            /* The above essentially makes the creation and execution of each thread to be sequential.
               The main thread creates one thread and waits for its 'joining' before creating another thread
               and thus defeating the purpose for which threads are being used.
            */
           
       }
     }
 
    printf("Product of the matrices:\n");
 
    for (i = 0; i < m; i++)
    {
      for (j = 0; j < r; j++)
        printf("%d\t", C[i][j]);
 
      printf("\n");
    }

 
  return 0;
}

/* The function that is executed by all threads */
/* Please note that the signature (return type and parameter type) of such functions (the ones to be executed
   by a thread created by pthread_create() is fixed. That means all the "parameters" that a thread may need
   should be "grouped" and its address should be passed */
void *runner(void *param) {
    struct v *data; /* the structure that holds our data */
    int l, sum = 0;

    data = (struct v *)param; /* the structure that holds our data */

        fprintf(stderr, "I am thread [%d][%d].\n", data->i, data->j);

    /*Row multiplied by column */
    for(l = 0; l< n; l++) {
        sum += A[data->i][l] * B[l][data->j];
    }

    /*assign the sum to its corresponding place */
    C[data->i][data->j] = sum;

        /* Free the data malloced in the main thread */
        free(param);

    /*Exit the thread */
    pthread_exit(0);
}