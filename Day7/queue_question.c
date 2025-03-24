# Producer-Consumer Problem Implementation in C

## Overview
This project implements a multi-threaded Producer-Consumer solution using a circular queue with the following key features:
- Supports multiple producer and consumer threads
- Dynamic thread management through an interactive manager thread
- Thread-safe queue operations using synchronization primitives
- Handles race conditions and thread synchronization

## Problem Description
The program simulates a classic Producer-Consumer scenario where:
- Multiple producer threads generate random integers
- Multiple consumer threads remove and process integers
- A shared circular queue (max 10 items) facilitates communication between producers and consumers
- A manager thread allows runtime configuration of producer and consumer threads

## Key Features
- Interactive thread management
- Thread-safe circular queue
- Synchronization using mutex and condition variables
- Avoid busy waiting
- Dynamic addition/removal of producer and consumer threads

## Compilation and Execution
```bash
gcc -o producerConsumer producerConsumer.c -lpthread
./producerConsumer
```

## Synchronization Mechanisms
- Mutex locks prevent race conditions
- Condition variables manage queue fullness and emptiness
- Producers wait when queue is full
- Consumers wait when queue is empty

## Manager Thread Interactions
The manager thread provides an interactive menu with options like:
- Add a producer thread
- Remove a producer thread
- Add a consumer thread
- Remove a consumer thread
- Display current thread statistics
- Exit the program

## Thread Behaviors
### Producer Threads
- Generate random integers
- Add items to the circular queue
- Wait if queue is full

### Consumer Threads
- Remove items from circular queue
- Process (print) items
- Wait if queue is empty

## Error Handling
- Prevents overflow of circular queue
- Manages thread creation and deletion
- Handles synchronization edge cases

## Limitations
- Maximum of 10 items in queue
- Maximum thread count limited by system resources

## Potential Improvements
- Configurable queue size
- More sophisticated thread management
- Advanced synchronization strategies

## Dependencies
- POSIX Threads (pthread) library

## License
[Specify your license here]

## Author
[Your Name]
```

Would you like me to modify anything in the README?