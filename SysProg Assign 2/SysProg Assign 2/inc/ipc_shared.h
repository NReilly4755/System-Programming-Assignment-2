#ifndef IPC_SHARED_H
#define IPC_SHARED_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>

// Constants
#define MAX_NAME 50
#define MAX_AGE 150
#define MIN_AGE 1
#define MIN_PEOPLE 1
#define MIN_TRIP 1
#define MAX_TRIPS 10
#define MSGTYPE_CLIENT 1
#define MAX_FULLNAME (MAX_NAME*2)
#define MAX_ADDRESS 100
#define SHM_KEY 0x1234
#define SEM_KEY 0x5678
#define MSG_KEY 0x9ABC
#define PERMISSIONS 0666


// Trip structure for shared memory
typedef struct {
    char destination[MAX_NAME];
    float price;
    int active; // 1 if trip is available, 0 if slot is empty
} Trip;

// Shared memory structure
typedef struct {
    int tripCount;
    Trip trips[MAX_TRIPS];
} SharedMemory;

// Message structure for message queue
typedef struct {
    long mtype;
    char firstName[MAX_NAME];
    char lastName[MAX_NAME];
    int age;
    char address[MAX_ADDRESS];
    char destination[MAX_NAME];
    int numPeople;
    float tripPrice;
} ClientMessage;

// Semaphore union for semctl
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

// Semaphore operations
void sem_lock(int semid);
void sem_unlock(int semid);
int create_semaphore(key_t key);
int get_semaphore(key_t key);
void remove_semaphore(int semid);

#endif // IPC_SHARED_H
