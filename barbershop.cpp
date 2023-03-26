// A barbershop consists of a waiting room with n chairs, and the barber room containing
// the barber chair. If there are no customers to be served, the barber goes to sleep. If
// a customer enters the barbershop and all chairs are occupied, then the customer leaves
// the shop. If the barber is busy, but chairs are available, then the customer sits in
// one of the free chairs. If the barber is asleep, the customer wakes up the barber.

// Library imports
#include <iostream>
#include <chrono>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <queue>

// Namespace declaration
using namespace std;

// Global variables 
int num_customers;
int max_chairs;
long int barber_wait_time;
long int customer_rate;
// Define barber status enum
enum enum_barber_status { AWAKE = true, ASLEEP = false};
enum_barber_status barber_status = ASLEEP;

// Mutex lock semaphore, and queue variables.
pthread_mutex_t mutex;
sem_t semaphore;
queue<int> customer_queue;

// Code for the barber, the "worker thread"
void* barber(void* arg) {
    // Acquire the mutex lock
    pthread_mutex_lock(&mutex);

    while (true) {
        // Check the customer queue.
        if (!customer_queue.empty()) { // There are customers in the queue.
            // Pop the first customer from the queue.
            int customer = customer_queue.front();
            customer_queue.pop();

            // Announce that a new customer is being processed.
            cout << "Customer #" << customer << " sits down in the barber's chair.\n";

            // If the barber is asleep, wake up the barber.
            if (barber_status == ASLEEP) {
                barber_status = AWAKE;
                cout << "Customer #" << customer << " has woken the barber.\n";
            }

            // Process the customer in the barber's chair.
            // Wait x time to "process the customer".
            sleep(barber_wait_time);
            // Customer is done being processed.
            cout << "Customer #" << customer << "'s haircut is finished. They leave the barbershop.\n";

        } else { // There are no customers in the queue.
            // No customers in the queue, so the barber falls asleep.
            if (barber_status == AWAKE ) {
                cout << "There are no customers waiting. The barber has fallen asleep.\n";
                barber_status = ASLEEP;
            }
        }
        // Release the mutex lock
        pthread_mutex_unlock(&mutex);
        // Signal that the barber is available
        sem_post(&semaphore);
    }
    return NULL;
}

int main() {
    // Initialize the mutex lock and semaphore variables
    pthread_mutex_init(&mutex, NULL);
    sem_init(&semaphore, 0, 1);

    // Ask user how many customers they'd like to simulate 
    cout << "Barbershop Simulation\n" << \
            "--------------------\n";
    cout << "How many customers would you like to simulate? (n): ";
    cin >> num_customers;
    cout << "How many chairs are in the waiting room? (n): ";
    cin >> max_chairs;
    cout << "How often should new customers appear? (seconds): ";
    cin >> customer_rate;
    cout << "How long should the barber spend on each customer? (seconds): ";
    cin >> barber_wait_time;
    cout << "\nBeginning simulation...\n" << \
            "-----------------------\n";

    // Start clock.
    auto start_time = chrono::high_resolution_clock::now();

    // Initalize barber worker thread.
    barber_status = ASLEEP;
    pthread_t barber_thread;
    pthread_create(&barber_thread, NULL, barber, 0);

    // Enqueue customers.
    for (int i = 1; i < num_customers + 1; i++) {
        if (customer_queue.size() >= max_chairs) {
            // Queue full, do not add.
            cout << "Customer #" << i << " arrives and sees that there is no room for them in the waiting room, so they leave.\n";
        } else {
            customer_queue.push(i);
            cout << "Customer #" << i << " arrives and sits in the waiting room. Current # of waiting customers: " << customer_queue.size() << "\n";

        }

        // Wait customer_rate amount seconds before another customer arrives.
        sleep(customer_rate);
    }

    // Hold the main thread until the customer queue is empty.
    while (!customer_queue.empty()) {}
    // Wait for the final customer to be processed.
    // TODO: fix this
    sleep(barber_wait_time);

    // Stop the chrono clock, print elapsed time in microseconds
    auto end_time = chrono::high_resolution_clock::now();
    auto elapsed_time = chrono::duration_cast<chrono::seconds>(end_time - start_time);
    cout << "\nEnd simulation...\n" << \
            "-------------------\n";
    cout << "The barber shop is now closed.\n";
    cout << "Elapsed simulation time: " << elapsed_time.count() << " seconds" << endl;

    // Destroy the mutex lock and semaphore variables
    pthread_mutex_destroy(&mutex);
    sem_destroy(&semaphore);

    return 0;
}
