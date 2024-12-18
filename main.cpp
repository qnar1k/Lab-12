#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>

using namespace std;

// Cross-platform type definitions for thread and lock management
#ifdef _COMPILE_WINDOWS
#include <Windows.h> 
typedef HANDLE THREADVAR;
typedef DWORD WINAPI THREADFUNCVAR;
typedef LPVOID THREADFUNCARGS;
typedef THREADFUNCVAR(*THREADFUNC)(THREADFUNCARGS);
typedef CRITICAL_SECTION THREAD_LOCK;
#endif

#ifdef _COMPILE_LINUX
#include <pthread.h>
#include <unistd.h>
typedef pthread_t THREADVAR;
typedef void* THREADFUNCVAR;
typedef void* THREADFUNCARGS;
typedef pthread_mutex_t THREAD_LOCK;
typedef unsigned long int DWORD_PTR;
typedef unsigned int DWORD;
typedef unsigned long long int uint64;
typedef long long int int64;
#endif

// Global variables
atomic<int> global_counter(0);  // Atomic to handle concurrent access safely
bool quitnow = false;
THREAD_LOCK global_lock;  // Lock to ensure only one thread accesses the global counter at a time

// Thread function declaration
THREADFUNCVAR ThreadFunction(THREADFUNCARGS arg);

// Cross-platform thread creation and stopping functions
THREADVAR PrepareThread(THREADFUNC f, THREADFUNCARGS arg);
void StopThread(THREADVAR t);
void InitThreadLock(THREAD_LOCK& t);
void LockThread(THREAD_LOCK& t);
void UnlockThread(THREAD_LOCK& t);
void sleep(int ms);

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Entry point
///////////////////////////////////////////////////////////////////////////////////////////////////////////
int main() {
    // Initialize the thread lock
    InitThreadLock(global_lock);

    // Create and start 5 threads
    const int num_threads = 5;
    THREADVAR threads[num_threads];
    
    // Start threads
    for (int i = 0; i < num_threads; ++i) {
        threads[i] = PrepareThread(ThreadFunction, nullptr);  // Pass nullptr as thread argument
    }

    // Main loop to increment and print global counter
    while (!quitnow) {
        LockThread(global_lock); // Lock the global counter before printing
        cout << "Main Thread: Global Counter = " << global_counter.load() << endl;
        UnlockThread(global_lock); // Unlock the global counter

        sleep(1000); // Sleep for a second
    }

    // Stop all threads
    for (int i = 0; i < num_threads; ++i) {
        StopThread(threads[i]);
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Thread Function
///////////////////////////////////////////////////////////////////////////////////////////////////////////
THREADFUNCVAR ThreadFunction(THREADFUNCARGS lpParam) {
    // Loop until the global counter exceeds a threshold
    while (!quitnow) {
        LockThread(global_lock);  // Lock the global counter
        global_counter++;  // Increment the global counter
        cout << "Thread: Incrementing global counter to " << global_counter.load() << endl;
        UnlockThread(global_lock);  // Unlock the global counter

        sleep(500);  // Sleep for 0.5 seconds
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Cross-platform thread management functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////
THREADVAR PrepareThread(THREADFUNC f, THREADFUNCARGS arg) {
#ifdef _COMPILE_LINUX
    pthread_t out;
    pthread_create(&out, NULL, f, arg);
    return out;
#endif
#ifdef _COMPILE_WINDOWS
    DWORD thrId;
    THREADVAR out = CreateThread(
        NULL,          // default security attributes
        0,             // use default stack size  
        (LPTHREAD_START_ROUTINE)f,  // thread function name
        arg,           // argument to thread function
        0,             // use default creation flags 
        &thrId         // returns the thread identifier
    );
    return out;
#endif
}

void StopThread(THREADVAR t) {
#ifdef _COMPILE_LINUX
    pthread_exit((void*)t);
#endif
#ifdef _COMPILE_WINDOWS
    TerminateThread(t, 0);
    CloseHandle(t);
#endif
}

void InitThreadLock(THREAD_LOCK& t) {
#ifdef _COMPILE_LINUX
    pthread_mutex_init(&t, NULL);
#endif
#ifdef _COMPILE_WINDOWS
    InitializeCriticalSection(&t);
#endif
}

void LockThread(THREAD_LOCK& t) {
#ifdef _COMPILE_LINUX
    pthread_mutex_lock(&t);
#endif
#ifdef _COMPILE_WINDOWS
    EnterCriticalSection(&t);
#endif
}

void UnlockThread(THREAD_LOCK& t) {
#ifdef _COMPILE_LINUX
    pthread_mutex_unlock(&t);
#endif
#ifdef _COMPILE_WINDOWS
    LeaveCriticalSection(&t);
#endif
}

void sleep(int ms) {
#ifdef _COMPILE_LINUX
    usleep(ms * 1000);  // usleep takes microseconds (1 millionth of a second)
#endif
#ifdef _COMPILE_WINDOWS
    Sleep(ms);  // Sleep takes milliseconds on Windows
#endif
}
