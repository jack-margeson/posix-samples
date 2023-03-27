// Consider a system with three smoker processes and one agent process. Each
// smoker continuously rolls a cigarette and then smokes it. But to roll and
// smoke a cigarette, the smoker needs three ingredients: tobacco, paper, and
// matches. 

// One of the smoker processes has paper, another has tobacco, and the
// third has matches. The agent has an infinite supply of all three materials.
// The agent places two of the ingredients on the table. The smoker who has the
// remaining ingredient then makes and smokes a cigarette, signaling the agent
// on completion. The agent then puts out another two of the three ingredients,
// and the cycle repeats.

// Library imports
#include <iostream>
#include <chrono>
#include <vector>
#include <queue>
#include <string>
#include <algorithm>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

using namespace std;

// Create a list of valid items for smokers and the vendor to have in their inventory.
const vector<string> VALID_ITEMS = {"paper", "tobacco", "matches"};

// Create a class to handle creating a "smoker".
// Contains information about the id of the smoker (provided in constructor)
// and the smoker's inventory (gains a random valid inventory item on creation)
class Smoker {
    public:
        int id;
        static int num_smokers;
        vector<string> inventory;

        Smoker(int id) {
            this->id = id;
            // Generate random number from 0 to 2.
            int random_int = rand() % 3;
            // Assign random item to smoker.
            this->inventory.push_back(VALID_ITEMS[random_int]);
        }

        // findNeeded() 
        // Given a smoker, returns a list of items that they need to roll a cig.
        vector<string> findNeeded() {
            vector<string> items_needed = VALID_ITEMS;
            vector<string>::iterator items_iterator;
            
            // For each item in the smoker's current inventory, remove it from the items needed
            for (string item : inventory) {
                // Remove the item from the needed items vector (they already have it).
                items_needed.erase(
                    remove(items_needed.begin(), items_needed.end(), item), 
                    items_needed.end()
                );
            }

            // Return the items needed.
            return items_needed;
        }

        string to_string() {
            string s = "";
            s += "Smoker #"; 
            s += std::to_string(this->id);
            s += "'s inventory: ";
            if (!this->inventory.empty()) {
                for (auto item : this->inventory) {
                    s += item + ", ";
                }
                s.resize(s.size() - 2);
            } else {
                s += "nothing.";
            }

            return s;
        }

};

// Global variables
int num_smokers;
long int agent_wait_time;
long int smoker_rate;
bool agent_active = false;

// Enum for agent status.
enum enum_agent_status {ASLEEP = 0, AWAKE = 1};
enum_agent_status agent_status = ASLEEP;

// Mutex lock and queue variables.
pthread_mutex_t mutex; // Locks shared variables
queue<Smoker> smoker_queue; // Shared variable

// Agent function 
// Creates an "agent", a worker which contains infinite materials
void* agent(void* arg) {
    while (agent_active) {
        // Take control of the mutex.
        pthread_mutex_lock(&mutex);

        // Check if there are smokers in the queue.
        if (!smoker_queue.empty()) {
            // Pop the first smoker off the queue.
            Smoker smoker = smoker_queue.front();
            smoker_queue.pop();
            
            // Release control of the mutex.
            pthread_mutex_unlock(&mutex);

            // Wake up the agent if they're asleep.
            if (agent_status == ASLEEP) {
                cout << "Smoker #" << smoker.id << " has woken the barber.\n";
                agent_status = AWAKE;
            }

            // Print the smokers inventory to the screen.
            cout << smoker.to_string() << "\n";

            // Find what the smoker needs for a cig and print to the screen.
            vector<string> items_needed = smoker.findNeeded();
            cout << "Smoker #" << smoker.id << " needs ";
            string items = "";
            for (string item : items_needed) {
                items += item + ", ";
            }
            items.resize(items.size() - 2);
            cout << items << " to roll a cigarette.\n";

            // Sleep to process the smoker.
            cout << "Agent is grabbing the requested items...\n";
            sleep(agent_wait_time);

            // Add items to smoker's inventory.
            for (string item : items_needed) {
                smoker.inventory.push_back(item);
            }
            cout << smoker.to_string() << "\n";
            cout << "Smoker #" << smoker.id << " smokes a cigarette and leaves.\n";

        } else { // The smoker queue is empty. 
            // Sleep the agent.
            if (agent_status == AWAKE) {
                cout << "There are no smokers in the queue. The agent has gone to sleep.\n";
                agent_status = ASLEEP;
            }
            // Release control of the mutex.
            pthread_mutex_unlock(&mutex);
        }
    }
    return NULL;
}

// Main program.
int main() {
    // Initialize the mutex lock
    pthread_mutex_init(&mutex, NULL);
    // Seed random number generator. 
    srand(time(0));

    // Get input from the user.
    cout << "Cigarette/Smoker Simulation\n" << \
            "--------------------\n";
    cout << "How many smokers would you like to simulate? (n): ";
    cin >> num_smokers;
    cout << "How often should new smokers appear? (seconds): ";
    cin >> smoker_rate;
    cout << "How long should the agent take to vend materials? (seconds): ";
    cin >> agent_wait_time;
    cout << "\nBeginning simulation...\n" << \
            "-----------------------\n";

    // Start clock.
    auto start_time = chrono::high_resolution_clock::now();

    // Start agent thread.
    agent_status = ASLEEP;
    agent_active = true;
    pthread_t agent_thread;
    pthread_create(&agent_thread, NULL, agent, 0);

    // Enqueue smokers.
    for (int i = 1; i < num_smokers + 1; i++) {
        // Lock the mutex,
        pthread_mutex_lock(&mutex);

        // Add smoker to the queue.
        cout << "Smoker #" << i << " has arrived.";
        smoker_queue.push(Smoker(i));

        // Unlock the mutex;
        pthread_mutex_unlock(&mutex);

        // Wait for the set amount of time before adding another smoker.
        sleep(smoker_rate);
    }

    // Hold the main thread until the smoker queue is empty.
    while (!smoker_queue.empty()) {}
    // Wait for the agent to vend to the last smoker.
    sleep(agent_wait_time);
    // Kill the agent.
    agent_active = false;

    // Stop the chrono clock, print elapsed time in microseconds
    auto end_time = chrono::high_resolution_clock::now();
    auto elapsed_time = chrono::duration_cast<chrono::seconds>(end_time - start_time);
    // Wait a second for the worker thread to finish being killed before printing results.
    sleep(1);
    // Print simulation results
    cout << "\nEnd simulation...\n" << \
            "-------------------\n";
    cout << "The smokeshop is now closed.\n";
    cout << "Elapsed simulation time: " << elapsed_time.count() << " seconds" << endl;

    // Free the mutex lock from memory.
    pthread_mutex_destroy(&mutex);

    // End program.
    return 0;
}