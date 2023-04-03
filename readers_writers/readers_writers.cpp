// Give neither priority, “the third readers-writers problem”: all readers and writers
// will be granted access to the resource in their order of arrival. If a writer arrives
// while readers are accessing the resource, it will wait until those readers free the
// resource, and then modify it. New readers arriving in the meantime will have to
// wait.

// Library imports
#include <iostream>
#include <chrono>
#include <queue>
#include <algorithm>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

using namespace std;

enum operation_type {READER, WRITER};

class Operation { 
    public: 
        int id;
        operation_type type;
        
        Operation(int id, operation_type type) { 
            this->id = id;
            this->type = type;
        }

        string to_string() {
            string s = "";
            s += "Operation #"; 
            s += std::to_string(this->id);
            s += ", ";
            switch(type) {
                case READER: 
                    s += "reader";
                    break;
                case WRITER: 
                    s += "writer";
                    break;
            }
            return s;
        }
};

// Global variables 
int worker_active = false;
// Muxex locks, semaphores, shared queues, ect.
sem_t operation_semaphore;
sem_t queue_semaphore;
queue<Operation> operation_queue;
int shared_int = 0; // This is the shared value we're going to be targeting.

// Operation worker thread code.
void* operation_handler(void* arg) {
    while (worker_active) {
        // Signal that we're working on an operation.
        sem_wait(&operation_semaphore);

        // Check if the queue has operations for us.
        if (!operation_queue.empty()) {
            // Pop an operation off the queue. 
            sem_wait(&queue_semaphore);
            Operation op = operation_queue.front();
            operation_queue.pop();
            sem_post(&queue_semaphore);

            // Handle the popped operation. 
            switch (op.type) {
                case READER:
                    // Reader code.
                    cout << op.to_string() << ", reads: " << shared_int << "\n";
                    break;
                case WRITER:
                    // Writer code.
                    cout << op.to_string() << ", increments shared value.\n";
                    shared_int++;
                    break;
                default:
                    break; 
            }
        }

        // We're done with the operation.
        sem_post(&operation_semaphore);
    }

    return NULL;
}




int main() {
    // Initalize the semaphore.
    sem_init(&operation_semaphore, 0, 1);
    sem_init(&queue_semaphore, 0, 1);


    // Start clock.
    auto start_time = chrono::high_resolution_clock::now();

    cout << "Starting value: " << shared_int << "\n";

    // Start the worker thread for operations.
    worker_active = true;
    pthread_t operation_handler_thread;
    pthread_create(&operation_handler_thread, NULL, operation_handler, 0);

    // Operation type queue:
    vector<operation_type> op_types = {READER, WRITER, READER, WRITER, READER, READER, READER};
    // Queue some operations.
    int i = 0;
    while (!op_types.empty()) {
        // Make operation.
        Operation op = Operation(i, op_types.front());
        // Remove from op_types vector.
        op_types.erase(op_types.begin());

        // Get queue_semaphore.
        sem_wait(&queue_semaphore);
        // Add new operation to queue.
        operation_queue.push(op);
        // Release queue_semaphore;
        sem_post(&queue_semaphore);
        i++;
    }


    // Hold program until queue is empty.
    while (!operation_queue.empty()) {};
    // Kill the operation handler.
    worker_active = false;

    // Stop the chrono clock, print elapsed time in microseconds
    auto end_time = chrono::high_resolution_clock::now();
    auto elapsed_time = chrono::duration_cast<chrono::microseconds>(end_time - start_time);
    // Wait a second for the worker thread to finish being killed before printing results.
    // Print elapese time.
    cout << "Elapsed time: " << elapsed_time.count() << " microseconds" << endl;

    // Free the semaphore from memory.
    sem_destroy(&operation_semaphore);
    sem_destroy(&queue_semaphore);
    // End program.
    return 0;
}