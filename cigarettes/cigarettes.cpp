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
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

using namespace std;

const string VALID_ITEMS[] = {"paper", "tobacco", "matches"};

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





int main() {
    // Seed random number generator. 
    srand(time(0));

    Smoker s1 = Smoker(0);
    cout << s1.to_string() << "\n";
    s1.inventory.clear();
    cout << s1.to_string() << "\n";
    Smoker s2 = Smoker(1);
    cout << s2.to_string();

    return 0;
}