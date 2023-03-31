// A single-lane bridge connects the two Vermont villages of North Turnbridge
// and South Turnbridge. Farmers in the two villages use this bridge to deliver
// their produce to the neighboring town. The bridge can become deadlocked if a
// northbound and a southbound farmer get on the bridge at the same time.
// (Vermont farmers are stubborn and are unable to back up).

// Library imports
#include <iostream>
#include <chrono>
#include <queue>
#include <algorithm>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

using namespace std;

enum direction_type {NORTHBOUND, SOUTHBOUND};

class Farmer {
    public: 
        int id;
        direction_type direction;

        Farmer(int id, direction_type direction) {
            this->id = id;
            this->direction = direction;
        }

        string to_string() {
            string s = "";
            s += "Farmer #";
            s += std::to_string(this->id);
            s += ", ";
            switch (direction) {
                case NORTHBOUND:
                    s += "northbound";
                    break;
                case SOUTHBOUND:
                    s += "southbound";
                break;
            }
            return s;
        }

};

// Global variables 
bool workers_active = false;
long int time_to_cross = 0;
long int arrival_rate = 0;
int num_farmers = 0;
// Create totals for purpose of demonstrating seperate threads
// for northbound and southbound farmers.
int num_northbound = 0;
int num_southbound = 0;
// Muxex locks, semaphores, shared queues, ect.
sem_t bridge_semaphore;
sem_t queue_semaphore;
queue<Farmer> farmer_queue;

// Farmer threads based on direction. 
// northbound_thread()
void* northbound_thread(void* arg) {
    while (workers_active) {
        sem_wait(&bridge_semaphore);
        // Check the farmer queue.
        sem_wait(&queue_semaphore);
        if (!farmer_queue.empty() && 
            farmer_queue.front().direction == NORTHBOUND
        ) {
            // Pop an operation off the queue.
            Farmer f = farmer_queue.front();
            farmer_queue.pop();
            sem_post(&queue_semaphore);

            // Handle the popped operation.
            // We know this farmer has a northbound operation.
            cout << "Now travelling: " << f.Farmer::to_string() << "\n";
            // Wait for the time to cross.
            sleep(time_to_cross);
            cout << f.Farmer::to_string() << " has finished crossing.\n";
            // Add to northbound total.
            num_northbound++;
         }
         // Free the queue semaphore in case the farmer queue is empty.
        sem_post(&queue_semaphore);
        sem_post(&bridge_semaphore);
    }
    return NULL;
}

// Farmer threads based on direction. 
// southbound_thread()
void* southbound_thread(void* arg) {
    while (workers_active) {
        sem_wait(&bridge_semaphore);
        // Check the farmer queue.
        sem_wait(&queue_semaphore);
        if (!farmer_queue.empty() && 
            farmer_queue.front().direction == SOUTHBOUND
        ) {
            // Pop an operation off the queue.
            Farmer f = farmer_queue.front();
            farmer_queue.pop();
            sem_post(&queue_semaphore);

            // Handle the popped operation.
            // We know this farmer has a southbound operation.
            cout << "Now traveling: " << f.Farmer::to_string() << "\n";
            // Wait for the time to cross.
            sleep(time_to_cross);
            cout << f.Farmer::to_string() << " has finished crossing.\n";
            // Add to southbound total.
            num_southbound++;
        }
        // Free the queue semaphore in case the farmer queue is empty.
        sem_post(&queue_semaphore);
        sem_post(&bridge_semaphore);
    }
    return NULL;
}

int main() {
    // Initalize semaphores.
    sem_init(&bridge_semaphore, 0, 1);
    sem_init(&queue_semaphore, 0, 1);

    // Ask user how many customers they'd like to simulate 
    cout << "Bridge Crossing Simulation\n" << \
            "--------------------\n";
    cout << "How many farmers would you like to simulate? (n): ";
    cin >> num_farmers;
       cout << "How often do farmers appear at the bridge? (seconds): ";
    cin >> arrival_rate;
    cout << "How long does it take to cross the bridge? (seconds): ";
    cin >> time_to_cross;
    cout << "\nBeginning simulation...\n" << \
            "-----------------------\n";

    // Start clock.
    auto start_time = chrono::high_resolution_clock::now();

    // Start both worker threads.
    workers_active = true;
    pthread_t northbound_thread_obj;
    pthread_t southbound_thread_obj;
    pthread_create(&northbound_thread_obj, NULL, northbound_thread, 0);
    pthread_create(&southbound_thread_obj, NULL, southbound_thread, 0);

    // Seed random number generator. 
    srand(time(0));

    // Enqueue farmers.
    for (int i = 0; i < num_farmers; i++) {
        sem_wait(&queue_semaphore);
        // Generate random number from 0 to 1.
        direction_type d = (rand() % 2) ? NORTHBOUND : SOUTHBOUND;
        Farmer f = Farmer(i, d);
        farmer_queue.push(f);
        cout << f.Farmer::to_string() << " has arrived.\n";
        sem_post(&queue_semaphore);

        // Wait for the next farmer;
        sleep(arrival_rate);
    }

    // Hold program until queue is empty.
    while (!farmer_queue.empty()) {};
    // Wait for the final farmer to cross.
    sleep(time_to_cross);
    // Kill the operation handler.
    workers_active = false;

     // Stop the chrono clock, print elapsed time in microseconds
    auto end_time = chrono::high_resolution_clock::now();
    auto elapsed_time = chrono::duration_cast<chrono::seconds>(end_time - start_time);
    // Wait a second for the worker threads to finish being killed before printing results.
    sleep(1);
    // Print simulation results
    cout << "\nEnd simulation...\n" << \
            "-------------------\n";
    cout << "The bridge is now closed for transport.\n";
    cout << "Elapsed simulation time: " << elapsed_time.count() << " seconds" << endl;
    cout << "Total number of northbound farmers: " << num_northbound << "\n";
    cout << "Total number of southbound farmers: " << num_southbound << "\n";

    // Destroy semaphores.
    sem_destroy(&bridge_semaphore);
    sem_destroy(&queue_semaphore);
    return 0;
}