//
// FILE               : client.c
// PROJECT            : SysProg Assing 2
// PROGRAMMER		  : Rodrigo P Gomes
// FIRST VERSION      : 2025-11-08
// DESCRIPTION        : This file is where client.c is located. The purpose of client.c is to gather infomration about a trip and clients then send the information
//                    : to the server using message queues and shared memory.
//

#include "ipc_shared.h"

//Lock semaphore (wait)
void sem_lock(int semid) {
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = -1; // P operation (wait/lock)
    sb.sem_flg = 0;
    
    //Performing the semaphore operation
    if (semop(semid, &sb, 1) == -1) {
        perror("semop lock");
        exit(1);
    }
}

//Unlock semaphore (signal)
void sem_unlock(int semid) {
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = 1;
    sb.sem_flg = 0;
    
    //Performing the semaphore operation
    if (semop(semid, &sb, 1) == -1) {
        perror("semop unlock");
        exit(1);
    }
}

//Create semaphore
int create_semaphore(key_t key) {
    int semid;
    union semun arg;
    
    //Create semaphore
    semid = semget(key, 1, IPC_CREAT | IPC_EXCL | PERMISSIONS);
    if (semid == -1) {
        perror("semget create");
        return -1;
    }
    
    //Initialize semaphore to 1 (unlocked)
    arg.val = 1;
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl SETVAL");
        return -1;
    }
    
    return semid;
}

//Get existing semaphore
int get_semaphore(key_t key) {
    int semid;
    
    //Get the semaphore
    semid = semget(key, 1, PERMISSIONS);
    if (semid == -1) {
        perror("semget get");
        return -1;
    }
    
    return semid;
}

//Remove semaphore
void remove_semaphore(int semid) {
    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID");
    }
}