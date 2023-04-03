// Some monkeys are trying to cross a ravine. A single rope traverses the ravine, and
// monkeys can cross hand-over-hand. Up to five monkeys can be hanging on the rope at
// any one time. If there are more than five, then the rope will break, and they will
// all die. Also, if eastward-moving monkeys encounter westward moving monkeys,
// all will fall off and die.

// Library imports.
#include <iostream>
#include <chrono>
#include <queue>
#include <algorithm>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

// Namespace declaration.
using namespace std;

// Enum declarations.
enum direction_type {EASTWARD, WESTWARD, NONE};
enum species_type {MONKEY, HUMAN};

// Primate class, handles data for both monkeys and humans
class Primate {
    private:
        int id;
        direction_type direction;
        species_type species;

    public: 
        Primate(int id, direction_type direction, species_type species) {
            this->id = id;
            this->direction = direction;
            this->species = species;
        }

        direction_type getDirection() {
            return this->direction;
        }

        string getDirectionString() {
            switch(this->direction) {
                case EASTWARD:
                    return "eastward";
                    break;
                case WESTWARD:
                    return "westward";
                    break;
                case NONE:
                    return "none";
                    break;
            }
            return "";
        }

        species_type getSpecies() {
            return this->species;
        }

        string getSpeciesString() {
            switch(this->species) {
                case MONKEY:
                    return "Monkey";
                    break;
                case HUMAN:
                    return "Human";
                    break;
            }
            return "";
        }

        string getPrimateIdentifier() {
            return getSpeciesString() + " #" + std::to_string(this->id);
        }

        string to_string() {
            return getPrimateIdentifier() + ", " + getDirectionString();
        }
};

// Global variables 
// Constants
const int MAX_CROSSING = 5;
const int MAX_CROSSING_EASTWARD = 3;
const int MAX_CROSSING_WESTWARD = 2;
// Worker information
bool worker_active = false;
long int time_to_cross = 0;
long int arrival_rate = 0;
string simulation_mode = "";
int num_primates = 0;
// Mutex locks, semaphores, shared queues, etc.
sem_t queue_semaphore;
sem_t crossing_semaphore;
queue<Primate> primate_queue;
// Shared struct of currently crossing primates.
struct  {
    public: 
        int total_east = 0;
        int total_west = 0;
        direction_type direction = NONE;
        int total() { return this->total_east + this->total_west; }
        void increment(Primate p) { (p.getDirection() == EASTWARD) ? total_east++ : total_west++; }
        void decrement(Primate p) { (p.getDirection() == EASTWARD) ? total_east-- : total_west--; }
} currently_crossing;

auto waitUntilSafe() {
    // If there are less than MAX_CROSSING currently crossing,
    // and if the primate is going the same direction as the 
    // currently crossing primates, then they are able to go.
    // Lock the queue semaphore.
    sem_wait(&queue_semaphore);
    // Init. a temp. primate.
    Primate temp_primate = primate_queue.front();
    // Post the queue semaphore.
    sem_post(&queue_semaphore);

    // Base case: if there are no primate crossing, it's safe to cross.
    // If we're a monkey, then we have to check if the currently crossing total is under MAX_CROSSING.
    // If it is, we also have to check if we're going the same direction as the currently crossing.
    // If both are true, we're safe to cross.
    // In the case we're a human, then we have to check if the currently crossing total is under MAX_CROSSING.
    // However, we do not have to check if we're going the same direction as the other humans, 
    // we only have to check if there are less than MAX_CROSSING_EASTWARD and MAX_CROSSING_WESTWARD depending on direction.
    if (!(currently_crossing.total() == 0)) {
        switch(temp_primate.getSpecies()) {
            case MONKEY:
                while (currently_crossing.total() == MAX_CROSSING) {
                    cout << currently_crossing.total();
                    // If the current amount of monkeys is equal to the max, we need to wait
                    // for the crossing semaphore until there is room.
                    sem_wait(&crossing_semaphore);
                }
                while (currently_crossing.direction != temp_primate.getDirection()) {
                    cout << temp_primate.to_string() << " needs to wait\n";
                    // If the current monkey does not fit in with the current direction,
                    // wait until all monkeys have crossed in the current direction before continuing.
                    sem_wait(&crossing_semaphore);
                }
                // We should be good to cross now. We'll add to the currently crossing in crossRavine().
                break;
            case HUMAN:
                while (currently_crossing.total() == MAX_CROSSING) {
                    // Wait for the semaphore if the max amount of current humans has been reached.
                    sem_wait(&crossing_semaphore);
                }
                // Switch based on direction of the human.
                switch(temp_primate.getDirection()) {
                    case EASTWARD:
                        while (currently_crossing.total_east == MAX_CROSSING_EASTWARD) {
                            // Wait for the semaphore if the max amount of eastward crossing humans has been reached.
                            sem_wait(&crossing_semaphore);
                        }
                        break;
                    case WESTWARD:
                        while (currently_crossing.total_west == MAX_CROSSING_WESTWARD) {
                            // Wait for the semaphore if the max amount of eastward crossing humans has been reached.
                            sem_wait(&crossing_semaphore);
                        }
                        break;
                    case NONE:
                        break;
                }
                // We should be good to cross now. We'll add to the currently crossing in crossRavine().
                break;
        }
    }
}

auto crossRavine() {
    // We know the primate next in line is cleared to cross the ravine.
    // Pop the first primate off of the queue.
    sem_wait(&queue_semaphore);
    Primate p = primate_queue.front();
    primate_queue.pop();
    sem_post(&queue_semaphore);
    // Update the currently crossing structure.
    // Update direction.
    if (currently_crossing.direction == NONE) {
        currently_crossing.direction = p.getDirection();
    }
    // Update count.
    currently_crossing.increment(p);

    // Output crossing string and wait the crossing amount of time.
    cout << p.to_string() << " is currently crossing.\n";
    sleep(time_to_cross);
    cout << p.to_string() << " has finished crossing.\n";

    // The primate we just dequeued has finished crossing the ravine.
    // Update the currently crossing structure.
    // Update count.
    currently_crossing.decrement(p);
    // Update direction.
    if (currently_crossing.total() == 0) {
        currently_crossing.direction = NONE;
    }
}

auto doneWithCrossing() {
    // We can signal that the next primate can cross.
    sem_post(&crossing_semaphore);
}

void* crossing_guard (void* arg) {
    while (worker_active) {
        if (!primate_queue.empty()) {
            // Wait until it is safe for the next primate to cross.
            waitUntilSafe();
            // Cross the ravine when it is safe to cross.
            crossRavine();
            // Signal that primate is done with crossing.
            doneWithCrossing();
        }
    }
    return NULL;
}


int main() {
    // Intalize semaphores.
    sem_init(&crossing_semaphore, 0, 1);
    sem_init(&queue_semaphore, 0, 1);

    // Ask user some questions about the simulation
    cout << "Primate Crossing Simulation\n" << \
            "--------------------\n";
    cout << "Would you like to run the simulation in monkey or human mode? (monkey/human): ";
    cin >> simulation_mode;
    cout << "How many primates would you like to simulate? (n): ";
    cin >> num_primates;
       cout << "How often do primates appear at the ravine? (seconds): ";
    cin >> arrival_rate;
    cout << "How long does it take to cross the ravine? (seconds): ";
    cin >> time_to_cross;
    cout << "\nBeginning simulation...\n" << \
            "-----------------------\n";

    // Seed random number generator.
    srand(time(0));

    // Start clock.
    auto start_time = chrono::high_resolution_clock::now();

    // Start "crossing guard" worker thread.
    worker_active = true;
    pthread_t crossing_guard_thread;
    pthread_create(&crossing_guard_thread, NULL, crossing_guard, 0);

    // Start adding primates to the queue.
    for (int i = 0; i < num_primates; i++) {
        sem_wait(&queue_semaphore);
        // Generate a random number from 0 to 1
        direction_type d = (rand() % 2) ? EASTWARD : WESTWARD;
        species_type s = (simulation_mode == "monkey") ? MONKEY : HUMAN;
        Primate p = Primate(i+1, d, s);
        primate_queue.push(p);
        cout << p.Primate::to_string() << " has arrived.\n";
        sem_post(&queue_semaphore);
        // Wait for the next primate.
        sleep(arrival_rate);
    }

    // Hold program until the primate vector is empty.
    while (!primate_queue.empty()) {}
    // Wait for the final primate to cross.
    sleep(time_to_cross);
    // Kill the crossing guard.
    worker_active = false;
    pthread_join(crossing_guard_thread, NULL);

    // Stop the chrono clock, print elapsed time in microseconds
    auto end_time = chrono::high_resolution_clock::now();
    auto elapsed_time = chrono::duration_cast<chrono::seconds>(end_time - start_time);
    // Wait a second for the worker threads to finish being killed before printing results.
    sleep(1);
    // Print simulation results
    cout << "\nEnd simulation...\n" << \
        "-------------------\n";
    cout << "All primates have crossed the ravine.\n";
    cout << "Elapsed simulation time: " << elapsed_time.count() << " seconds" << endl;


    // Destroy semaphores
    sem_destroy(&crossing_semaphore);
    sem_destroy(&queue_semaphore);
    return 0;
}