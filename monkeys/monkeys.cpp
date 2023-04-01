// Some monkeys are trying to cross a ravine. A single rope traverses the ravine, and
// monkeys can cross hand-over-hand. Up to five monkeys can be hanging on the rope at
// any one time. If there are more than five, then the rope will break, and they will
// all die. Also, if eastward-moving monkeys encounter westward moving monkeys,
// all will fall off and die.

// Library imports
#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

using namespace std;

enum direction_type {EASTWARD, WESTWARD};

class Monkey {
    public:
        // non-static public variables
        int id;
        direction_type direction;

        Monkey(int id, direction_type direction) {
            this->id = id;
            this->direction = direction;
        }

        string getMonkeyIdentifier() {
            return "Monkey #" + std::to_string(this->id);
        }

        string getDirection() {            
            switch (this->direction) {
                case EASTWARD:
                    return "eastward";
                    break;
                case WESTWARD:
                    return "westward";
                    break;
            } 
            return "";
        }

        string to_string() {
            string s = "";
            s += getMonkeyIdentifier();
            s += ", ";
            s += getDirection();
            return s;
        }
};

// Global variables 
bool workers_active = false;
long int time_to_cross = 0;
long int arrival_rate = 0;
int num_monkeys;
const int MAX_MONKEYS = 5;
// Mutex locks, semaphores, shared queues
sem_t vector_semaphore;
sem_t crossing_semaphore;
vector<Monkey> monkey_vector;

auto waitUntilSafe () {
    // Wait until the crossing_semaphore is posted.
    sem_wait(&crossing_semaphore);
}

auto crossRavine () {
    // Lock the queue semaphore.
    // If we're in this function, the rope is empty. We can 
    // accept 5 monkeys at the same time, provided that they're
    // going the right direction. Find the first 5 monkeys in the queue
    // that match the first direction, then send them. If there are less 
    // than 5, let them go.

    // Lock the main vector from operations.
    sem_wait(&vector_semaphore);

    // Get the first monkey in the queue, the "leader"
    // and add to the currently crossing vector
    vector<Monkey> currently_crossing;
    direction_type currently_crossing_direction;
    cout << monkey_vector.front().Monkey::getMonkeyIdentifier() <<
                        " is getting ready to cross.\n";
    currently_crossing.push_back(monkey_vector.front());
    currently_crossing_direction = monkey_vector.front().direction;
    monkey_vector.erase(monkey_vector.begin());

    // For the rest of the monkeys in the monkey vector/queue,
    // check if they're going the same direction as the "leader"
    // monkey at the front of the currently crossing vector.
    // If they are, we can add them to the currently crossing group
    // as long as there are not more than MAX_MONKEYS in the group.
    // Otherwise, state the monkey is going in the same direction, 
    // but cannot come due to the limit on the rope.
    vector<Monkey> remaining_monkeys;
    for (int i = 0; i < monkey_vector.size(); i++) {
        if (monkey_vector[i].direction == currently_crossing_direction) {
            if (currently_crossing.size() < MAX_MONKEYS) {
                cout << monkey_vector[i].getMonkeyIdentifier() << 
                    " is getting ready to cross.\n";
                currently_crossing.push_back(monkey_vector[i]);
            } else {
                cout << monkey_vector[i].getMonkeyIdentifier() << 
                    " is waiting to cross " << monkey_vector[i].getDirection() <<
                    ", but the current group is full.\n";
                remaining_monkeys.push_back(monkey_vector[i]);
            }
        } else {
            remaining_monkeys.push_back(monkey_vector[i]);
        }
    }
    monkey_vector = remaining_monkeys;


    // Free the main vector for writing.
    sem_post(&vector_semaphore);


    // Cross all monkeys going the same direction, in n=MAX_MONKEYS group
    cout << "A group of monkeys (";
    string curr = "";
    for (Monkey m : currently_crossing) {
        curr += m.Monkey::getMonkeyIdentifier();
        curr += ", ";
    }
    curr.resize(curr.size() - 2);
    cout << curr << 
            ") is crossing the ravine going " <<
            currently_crossing.front().Monkey::getDirection()
            << ".\n";

    // Wait the ravine crossing time.
    sleep(time_to_cross);

    // Print that the group has made it across.
    cout << "The group of monkeys (" <<
            curr <<
            ") has made it across the ravine.\n";
}

auto doneWithCrossing() {
    // Post the crossing semaphore.
    sem_post(&crossing_semaphore);
}

void* crossing_guard (void* arg) {
    while (workers_active) {
        if (!monkey_vector.empty()) {
            // Wait until it is safe for monkeys to cross.
            waitUntilSafe();
            // If it's safe to cross, cross the ravine.
            crossRavine();
            // Signal that the current group of monkeys is done crossing.
            doneWithCrossing();
        }
    }

    return NULL;
}


int main() {
    // Intalize semaphores.
    sem_init(&crossing_semaphore, 0, 1);
    sem_init(&vector_semaphore, 0, 1);

    // Ask user how many monkeys they'd like to simulate 
    cout << "Monkey Crossing Simulation\n" << \
            "--------------------\n";
    cout << "How many monkeys would you like to simulate? (n): ";
    cin >> num_monkeys;
       cout << "How often do monkeys appear at the ravine? (seconds): ";
    cin >> arrival_rate;
    cout << "How long does it take to cross the ravine? (seconds): ";
    cin >> time_to_cross;
    cout << "\nBeginning simulation...\n" << \
            "-----------------------\n";

    // Start clock.
    auto start_time = chrono::high_resolution_clock::now();


    // Start "crossing guard" worker thread.
    workers_active = true;
    pthread_t crossing_guard_thread;
    pthread_create(&crossing_guard_thread, NULL, crossing_guard, 0);

    // Seed random number generator.
    srand(time(0));
    
    // Start adding monkeys to the vector.
    for (int i = 0; i < num_monkeys; i++) {
        sem_wait(&vector_semaphore);
        // Generate a random number from 0 to 1
        direction_type d = (rand() % 2) ? EASTWARD : WESTWARD;
        Monkey m = Monkey(i+1, d);
        monkey_vector.push_back(m);
        cout << m.Monkey::to_string() << " has arrived.\n";
        sem_post(&vector_semaphore);

        // Wait for the next monkey
        sleep(arrival_rate);
    }

    // Hold program until the monkey vector is empty.
    while (!monkey_vector.empty()) {}
    // Wait for the final monkey group to cross.
    sleep(time_to_cross);
    // Kill the operation handler crossing guard.
    workers_active = false;

    // Stop the chrono clock, print elapsed time in microseconds
    auto end_time = chrono::high_resolution_clock::now();
    auto elapsed_time = chrono::duration_cast<chrono::seconds>(end_time - start_time);
    // Wait a second for the worker threads to finish being killed before printing results.
    sleep(1);
    // Print simulation results
    cout << "\nEnd simulation...\n" << \
        "-------------------\n";
    cout << "All monkeys have crossed the ravine.\n";
    cout << "Elapsed simulation time: " << elapsed_time.count() << " seconds" << endl;

    // Destroy semaphores
    sem_destroy(&crossing_semaphore);
    sem_destroy(&vector_semaphore);
    return 0;
}