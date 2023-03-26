#include <iostream>
#include <chrono>
#include <queue>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

// Define the maximum size of the queue
const int MAX_QUEUE_SIZE = 10;

// Define the queue and its mutex lock
std::queue<int> task_queue;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

// Define the semaphores for counting tasks and available slots in the queue
sem_t task_count, queue_slots;

// Define the worker thread function
void* worker_thread(void* arg) {
  int task;

  while (true) {
    // Wait for a task to become available
    sem_wait(&task_count);

    // Acquire the mutex lock on the queue
    pthread_mutex_lock(&queue_mutex);

    // Get the next task from the queue
    task = task_queue.front();
    task_queue.pop();

    // Release the mutex lock on the queue
    pthread_mutex_unlock(&queue_mutex);

    // Signal that a queue slot is now available
    sem_post(&queue_slots);

    // Process the task
    std::cout << "Processing task " << task << std::endl;
  }

  return NULL;
}

int main() {
  // Initialize the semaphores
  sem_init(&task_count, 0, 0);
  sem_init(&queue_slots, 0, MAX_QUEUE_SIZE);

  // Create the worker thread
  pthread_t worker_thread_id;
  pthread_create(&worker_thread_id, NULL, worker_thread, NULL);

  // Generate some sample tasks
  for (int i = 0; i < 100; i++) {
    // Wait for an available queue slot
    sem_wait(&queue_slots);

    // Acquire the mutex lock on the queue
    pthread_mutex_lock(&queue_mutex);

    // Add the task to the queue
    task_queue.push(i);

    // Release the mutex lock on the queue
    pthread_mutex_unlock(&queue_mutex);

    // Signal that a task is now available
    sem_post(&task_count);

    // Wait for a short time before generating the next task
    sleep(0);
  }

  // Signal that the worker thread should exit
  sem_post(&task_count);

  // Wait for the worker thread to finish
  pthread_join(worker_thread_id, NULL);

  // Clean up the semaphores and mutex lock
  sem_destroy(&task_count);
  sem_destroy(&queue_slots);
  pthread_mutex_destroy(&queue_mutex);

  return 0;
}
