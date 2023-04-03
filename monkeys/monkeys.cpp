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
int num_monkeys = 0;
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
    if (!currently_crossing.total() == 0) {
        switch(temp_primate.getSpecies()) {
            case MONKEY:
                while (currently_crossing.total() == MAX_CROSSING) {
                    // If the current amount of monkeys is equal to the max, we need to wait
                    // for the crossing semaphore until there is room.
                    sem_wait(&crossing_semaphore);
                }
                while (currently_crossing.direction != temp_primate.getDirection()) {
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
}

auto doneWithCrossing() {
    // The primate we just dequeued has finished crossing the ravine.
    // Update the currently crossing structure.
    // Update count.
    currently_crossing.decrement();
    // We can update currently_crossing and then signal that the next primate
    // can cross.
    sem_post(&crossing_semaphore);
}

void* crossing_guard (void* arg) {
    // Wait until it is safe for the next primate to cross.
    waitUntilSafe();
    // Cross the ravine when it is safe to cross.
    crossRavine();
    // Signal that primate is done with crossing.
    doneWithCrossing();
}


int main() {

    return 0;
}