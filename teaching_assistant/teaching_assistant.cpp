// The university computer science department has a teaching assistant (TA) who
// helps undergraduate students with their programming assignments during
// regular office hours. The TA’s office is rather small and has room for only one
// desk with a chair and computer. There are three chairs in the hallway outside
// the office where students can sit and wait if the TA is currently helping
// another student. 

//When there are no students who need help during office
// hours, the TA sits at the desk and takes a nap. If a student arrives during
// office hours and finds the TA sleeping, the student must awaken the TA to ask
// for help. If a student arrives and finds the TA currently helping another
// student, the student sits on one of the chairs in the hallway and waits. If no
// chairs are available, the student will come back later.

// Library imports
#include <iostream>
#include <chrono>
#include <queue>
#include <pthread.h>
#include <unistd.h>

// Namespace declaration
using namespace std;

// Global variables 
int num_students;
const int max_chairs = 3;
long int teaching_assistant_wait_time;
long int student_rate;
// Define teaching assistant status enum
enum enum_teaching_assistant_status { AWAKE = true, ASLEEP = false};
enum_teaching_assistant_status teaching_assistant_status = ASLEEP;
// Define worker process management variable 
bool worker_active = false;

// Mutex lock semaphore, and queue variables.
pthread_mutex_t mutex;
queue<int> student_queue;

// Code for the teaching assistant (worker thread)
void* teaching_assistant(void* arg) {
   while (worker_active) {
        // Lock the mutex (nodifying the student queue).
        pthread_mutex_lock(&mutex);
        // Check the student queue.
        if (!student_queue.empty()) { // There are customers in the queue.
            // Pop the first customer from the queue.
            int student = student_queue.front();
            student_queue.pop();

            // Unlock the mutex (done with modifications to the customer queue)
            pthread_mutex_unlock(&mutex);
            // Announce that a new student is being processed.
            cout << "Student #" << student << " sits down with the teaching assistant.\n";

            // If the TA is asleep, wake up the TA.
            if (teaching_assistant_status == ASLEEP) {
                teaching_assistant_status = AWAKE;
                cout << "Student #" << student << " has woken the teaching assistant.\n";
            }

            // Process the student currently with the TA.
            // Wait x time to "process the stydent".
            sleep(teaching_assistant_wait_time);
            // Student is done being processed.
            cout << "Student #" << student << " gets their questions answered. They leave office hours.\n";

        } else { // There are no students in the queue.
            // No students in the queue, so the teaching assistant falls asleep.
            if (teaching_assistant_status == AWAKE ) {
                cout << "There are no students waiting. The teaching assistant has fallen asleep.\n";
                teaching_assistant_status = ASLEEP;
            }

            // Unlock the mutex (done with modifications to the customer queue)
            pthread_mutex_unlock(&mutex);
        }
    }
    return NULL;
}

int main() {
    // Initialize the mutex lock
    pthread_mutex_init(&mutex, NULL);

    // Ask user how many customers they'd like to simulate 
    cout << "Office Hours Simulation\n" << \
            "--------------------\n";
    cout << "How many students would you like to simulate? (n): ";
    cin >> num_students;
    cout << "How often should the students appear? (seconds): ";
    cin >> student_rate;
    cout << "How long should the teaching assistant spend with each student? (seconds): ";
    cin >> teaching_assistant_wait_time;
    cout << "\nBeginning simulation...\n" << \
            "-----------------------\n";

    // Start clock.
    auto start_time = chrono::high_resolution_clock::now();

    // Initalize barber worker thread.
    teaching_assistant_status = ASLEEP;
    worker_active = true;
    pthread_t teaching_assistant_thread;
    pthread_create(&teaching_assistant_thread, NULL, teaching_assistant, 0);

    // Enqueue students.
    for (int i = 1; i < num_students + 1; i++) {
        // Lock the mutex (nodifying the students queue).
        pthread_mutex_lock(&mutex);

        if (student_queue.size() >= max_chairs) {
            // Queue full, do not add.
            cout << "Student #" << i << " arrives and sees that there is no room for them in the hallway, so they leave.\n";
        } else {
            student_queue.push(i);
            cout << "Student #" << i << " arrives and sits in the hallway. Current # of waiting students: " << student_queue.size() << "\n";
        }

        // Unlock the mutex (done with modifications to the student queue)
        pthread_mutex_unlock(&mutex);

        // Wait student_rate amount seconds before another student arrives.
        sleep(student_rate);
    }

    // Hold the main thread until the student queue is empty.
    while (!student_queue.empty()) {}
    // Wait an additional time for the TA to finish with the last student.
    sleep(teaching_assistant_wait_time);
    // Kill the teaching assistant thread.
    worker_active = false;

    // Stop the chrono clock, print elapsed time in microseconds
    auto end_time = chrono::high_resolution_clock::now();
    auto elapsed_time = chrono::duration_cast<chrono::seconds>(end_time - start_time);
    // Wait a second for the worker thread to finish being killed before printing results.
    sleep(1);
    // Print simulation results
    cout << "\nEnd simulation...\n" << \
            "-------------------\n";
    cout << "Office hours are now over.\n";
    cout << "Elapsed simulation time: " << elapsed_time.count() << " seconds" << endl;

    // Free the mutex lock.
    pthread_mutex_destroy(&mutex);

    return 0;
}
