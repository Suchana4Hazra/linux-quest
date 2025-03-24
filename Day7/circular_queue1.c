#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define MAX_QUEUE_SIZE 10
#define MAX_PRODUCER_THREADS 10
#define MAX_CONSUMER_THREADS 10
#define MAX_SLEEP_TIME 5

// Circular Queue Structure
typedef struct
{
    int items[MAX_QUEUE_SIZE];
    int front, rear;
    pthread_mutex_t mutex;
    pthread_cond_t empty, full;
} CircularQueue;

// Function prototypes
void initQueue(CircularQueue *q);
void enQ(CircularQueue *q, int item);
int deQ(CircularQueue *q);
int isFull(CircularQueue *q);
int isEmpty(CircularQueue *q);
void clearResources();
void deleteProducer();
void deleteConsumer();

// Global variables
CircularQueue queue;
pthread_t producerThreads[MAX_PRODUCER_THREADS];
pthread_t consumerThreads[MAX_CONSUMER_THREADS];
pthread_t managerThread;
int numProducers = 0, numConsumers = 0;

// Initialize the queue
void initQueue(CircularQueue *q)
{
    q->front = q->rear = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->empty, NULL);
    pthread_cond_init(&q->full, NULL);
}

// Add item to queue
void enQ(CircularQueue *q, int item)
{
    fprintf(stderr,"\t\t\t\tCalled enQ!\n");
    fprintf(stderr,"\t\t\t\tenQ locking!\n");
    pthread_mutex_lock(&q->mutex);
    while (isFull(q))
    {
    	fprintf(stderr,"\t\t\t\tenQ Waiting!\n");
        pthread_cond_wait(&q->full, &q->mutex);
    }
    q->items[q->rear] = item;
    q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
    fprintf(stderr,"\t\t\t\tenQ signalling!\n");
    pthread_cond_signal(&q->empty);
    fprintf(stderr,"\t\t\t\tenQ unlocking!\n");
    pthread_mutex_unlock(&q->mutex);
}

// Remove item from queue
int deQ(CircularQueue *q)
{
    fprintf(stderr,"\t\t\t\t\t\tCalled deQ!\n");
    fprintf(stderr,"\t\t\t\t\t\tdeQ locking!\n");
    pthread_mutex_lock(&q->mutex);
    while (isEmpty(q))
    {
    	fprintf(stderr,"\t\t\t\t\t\tdeQ waiting!\n");
        pthread_cond_wait(&q->empty, &q->mutex);
    }
    int item = q->items[q->front];
    q->front = (q->front + 1) % MAX_QUEUE_SIZE;
    fprintf(stderr,"\t\t\t\t\t\tdeQ signalling!\n");
    pthread_cond_signal(&q->full);
    fprintf(stderr,"\t\t\t\t\t\tdeQ unlocking!\n");
    pthread_mutex_unlock(&q->mutex);
    return item;
}

// Check if queue is full
int isFull(CircularQueue *q)
{
    return ((q->rear + 1) % MAX_QUEUE_SIZE == q->front);
}

// Check if queue is empty
int isEmpty(CircularQueue *q)
{
    return (q->front == q->rear);
}

// Producer function
void *producer(void *data)
{
    int producerId = *((int *)data);
    while (1)
    {
        // Produce random number of items
        srand(time(NULL));
        int numItems = rand() % (MAX_QUEUE_SIZE - 1) + 1; // Produces 1 to MAX_QUEUE_SIZE items
        printf("\n");
        for (int i = 0; i < numItems; ++i)
        {
            int item = rand() % 100;
            enQ(&queue, item);
            printf("\t\t\t\tProducer %d produced %d/%d item: %d\n", producerId, i+1, numItems, item);
        }
    }

    sleep(rand() % MAX_SLEEP_TIME + 1);
    return NULL;
}

// Consumer function
void *consumer(void *data)
{
    srand(time(NULL));
    int consumerId = *((int *)data);
    while (1)
    {
        // Consume random number of items
        int numItems = rand() % (MAX_QUEUE_SIZE - 1) + 1; // Consumes 1 to MAX_QUEUE_SIZE items
        printf("\n");
        for (int i = 0; i < numItems; ++i)
        {
            int item = deQ(&queue);
            printf("\t\t\t\t\t\tConsumer %d consumed %d/%d item: %d\n", consumerId, i+1, numItems, item);
        }
        sleep(rand() % MAX_SLEEP_TIME + 1);
    }
    return NULL;
}

// Clear resources function
void clearResources()
{
    // Clear producer threads
    for (int i = 0; i < numProducers; ++i)
    {
        pthread_cancel(producerThreads[i]);
    }
    // Clear consumer threads
    for (int i = 0; i < numConsumers; ++i)
    {
        pthread_cancel(consumerThreads[i]);
    }
    // Destroy mutex and condition variables
    pthread_mutex_destroy(&queue.mutex);
    pthread_cond_destroy(&queue.empty);
    pthread_cond_destroy(&queue.full);
    printf("All threads and resources cleared.\n");
    // Reset thread counts
    numProducers = 0;
    numConsumers = 0;
}

// Delete producer function
void deleteProducer()
{
    if (numProducers > 0)
    {
        pthread_cancel(producerThreads[numProducers - 1]);
        printf("Producer thread %d deleted.\n", numProducers);
        numProducers--;
    }
    else
    {
        printf("No producer threads to delete.\n");
    }
}

// Delete consumer function
void deleteConsumer()
{
    if (numConsumers > 0)
    {
        pthread_cancel(consumerThreads[numConsumers - 1]);
        printf("Consumer thread %d deleted.\n", numConsumers);
        numConsumers--;
    }
    else
    {
        printf("No consumer threads to delete.\n");
    }
}

// Manager thread function
void *manager(void *data)
{
    char choice;
    printf("Welcome to the manager thread!\n");
    do
    {
        printf("Menu:\n");
        printf("1. Add Producer\n");
        printf("2. Add Consumer\n");
        printf("3. Delete Producer\n");
        printf("4. Delete Consumer\n");
        printf("5. Clear All Threads, Resources and Exit.\n");
        printf("Enter your choice: ");
        scanf(" %c", &choice);

        switch (choice)
        {
        case '1':
            if (numProducers < MAX_PRODUCER_THREADS)
            {
                pthread_create(&producerThreads[numProducers], NULL, producer, &numProducers);
                numProducers++;
            }
            else
            {
                printf("Cannot add more producer threads.\n");
            }
            break;
        case '2':
            if (numConsumers < MAX_CONSUMER_THREADS)
            {
                pthread_create(&consumerThreads[numConsumers], NULL, consumer, &numConsumers);
                numConsumers++;
            }
            else
            {
                printf("Cannot add more consumer threads.\n");
            }
            break;
        case '3':
            deleteProducer();
            break;
        case '4':
            deleteConsumer();
            break;
        case '5':
            clearResources();
            printf("Exiting manager thread.\n");
            break;
            break;
        default:
            printf("Invalid choice!\n");
        }
    } while (choice != '5');

    return NULL;
}

int main()
{
    initQueue(&queue);
    pthread_create(&managerThread, NULL, manager, NULL);

    pthread_join(managerThread, NULL); // Wait for manager thread to finish

    return 0;
}
