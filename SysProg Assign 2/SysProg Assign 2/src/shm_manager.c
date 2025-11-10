#include "common.h"

int shmid = -1;
int semid = -1;
SharedMemory *shm = NULL;

void cleanup() {
    if (shm != NULL) {
        shmdt(shm);
    }
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, NULL);
        printf("Shared memory removed.\n");
    }
    if (semid != -1) {
        remove_semaphore(semid);
        printf("Semaphore removed.\n");
    }
}

void create_shared_memory() {
    if (shmid != -1) {
        printf("Shared memory already exists!\n");
        return;
    }
    
    //Create shared memory
    shmid = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | IPC_EXCL | 0666);
    if (shmid == -1) {
        perror("shmget create");
        return;
    }
    
    //Attach shared memory
    shm = (SharedMemory *)shmat(shmid, NULL, 0);
    if (shm == (void *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        shmid = -1;
        return;
    }
    
    //Create semaphore
    semid = create_semaphore(SEM_KEY);
    if (semid == -1) {
        shmdt(shm);
        shmctl(shmid, IPC_RMID, NULL);
        shmid = -1;
        shm = NULL;
        return;
    }
    
    printf("Shared memory created successfully!\n");
    
    //Initialize with some default trips
    sem_lock(semid);
    shm->tripCount = 0;
    for (int i = 0; i < MAX_TRIPS; i++) {
        shm->trips[i].active = 0;
    }
    sem_unlock(semid);
    
    //Add some sample trips
    char choice;
    printf("\nWould you like to add trips now? (y/n): ");
    scanf(" %c", &choice);
    getchar(); 
    
    if (choice == 'y' || choice == 'Y') {
        while (1) {
            if (shm->tripCount >= MAX_TRIPS) {
                printf("Maximum trips reached!\n");
                break;
            }
            
            Trip newTrip;
            printf("\nEnter destination: ");
            fgets(newTrip.destination, MAX_NAME, stdin);
            newTrip.destination[strcspn(newTrip.destination, "\n")] = 0;
            
            if (strlen(newTrip.destination) == 0) {
                printf("Destination cannot be empty!\n");
                continue;
            }
            
            printf("Enter price: ");
            if (scanf("%f", &newTrip.price) != 1 || newTrip.price < 0) {
                printf("Invalid price!\n");
                while (getchar() != '\n');
                continue;
            }
            getchar();
            
            newTrip.active = 1;
            
            sem_lock(semid);
            shm->trips[shm->tripCount] = newTrip;
            shm->tripCount++;
            sem_unlock(semid);
            
            printf("Trip added successfully!\n");
            
            printf("Add another trip? (y/n): ");
            scanf(" %c", &choice);
            getchar();
            
            if (choice != 'y' && choice != 'Y') {
                break;
            }
        }
    }
}

void read_shared_memory() {
    if (shmid == -1 || shm == NULL) {
        printf("Shared memory not created yet!\n");
        return;
    }
    
    sem_lock(semid);
    
    printf("\n=== Available Trips ===\n");
    printf("Total trips: %d\n", shm->tripCount);
    
    if (shm->tripCount == 0) {
        printf("No trips available.\n");
    } else {
        for (int i = 0; i < shm->tripCount; i++) {
            if (shm->trips[i].active) {
                printf("%d. %s - $%.2f\n", i + 1, 
                       shm->trips[i].destination, 
                       shm->trips[i].price);
            }
        }
    }
    
    sem_unlock(semid);
}

void kill_shared_memory() {
    if (shmid == -1 || shm == NULL) {
        printf("Shared memory not created yet!\n");
        return;
    }
    
    printf("\nAttempting rogue write to kill shared memory...\n");
    
    //Attempt to write to out-of-range memory address. This is for corruption in memory.
    char *rogue_ptr = (char *)shm + sizeof(SharedMemory) + 1000;
    
    printf("Writing to address: %p (out of bounds)\n", (void *)rogue_ptr);
    
    //This may cause segmentation fault or undefined behavior
    *rogue_ptr = 'X';
    
    printf("Rogue write completed (this may have corrupted memory).\n");
    printf("Cleaning up shared memory...\n");
    
    cleanup();
    shmid = -1;
    shm = NULL;
    semid = -1;
}

void display_menu() {
    printf("\n=== Shared Memory Manager ===\n");
    printf("1. Create shared memory\n");
    printf("2. Read shared memory\n");
    printf("3. Kill shared memory with rogue write\n");
    printf("4. Exit\n");
    printf("Enter choice: ");
}

int main(void) {
    int choice;
    
    printf("Shared Memory Manager: Starting...\n");
    
    while (1) {
        display_menu();
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input!\n");
            while (getchar() != '\n');
            continue;
        }
        getchar();
        
        switch (choice) {
            case 1:
                create_shared_memory();
                break;
            case 2:
                read_shared_memory();
                break;
            case 3:
                kill_shared_memory();
                break;
            case 4:
                printf("Exiting...\n");
                cleanup();
                exit(0);
            default:
                printf("Invalid choice!\n");
        }
    }
    
    return 0;
}