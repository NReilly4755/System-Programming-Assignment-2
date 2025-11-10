#include "common.h"

int msgid = -1;
int shmid = -1;
int semid = -1;
SharedMemory *shm = NULL;

void cleanup() {
    if (shm != NULL) {
        shmdt(shm);
    }
}

int validate_name(char *name) {
    if (strlen(name) == 0) {
        return 0;
    }
    
    //Check if name contains only letters and spaces
    for (int i = 0; i < strlen(name); i++) {
        if (!isalpha(name[i]) && name[i] != ' ') {
            return 0;
        }
    }
    
    return 1;
}

int validate_age(int age) {
    return age > 0 && age < 150;
}

void get_client_data(ClientMessage *msg) {
    char fullName[MAX_NAME * 2];
    char *token;
    
    //Get first and last name
    while (1) {
        printf("\nEnter client name (First Last): ");
        fgets(fullName, sizeof(fullName), stdin);
        fullName[strcspn(fullName, "\n")] = 0;
        
        if (!validate_name(fullName)) {
            printf("Invalid name! Use only letters and spaces.\n");
            continue;
        }
        
        //Split into first and last name
        token = strtok(fullName, " ");
        if (token == NULL) {
            printf("Please enter both first and last name!\n");
            continue;
        }
        strncpy(msg->firstName, token, MAX_NAME - 1);
        msg->firstName[MAX_NAME - 1] = '\0';
        
        token = strtok(NULL, " ");
        if (token == NULL) {
            printf("Please enter both first and last name!\n");
            continue;
        }
        strncpy(msg->lastName, token, MAX_NAME - 1);
        msg->lastName[MAX_NAME - 1] = '\0';
        
        break;
    }
    
    //Get age
    while (1) {
        printf("Enter age: ");
        if (scanf("%d", &msg->age) != 1) {
            printf("Invalid input! Please enter a number.\n");
            while (getchar() != '\n');
            continue;
        }
        getchar();
        
        if (!validate_age(msg->age)) {
            printf("Invalid age! Please enter a value between 1 and 149.\n");
            continue;
        }
        
        break;
    }
    
    //Get address
    while (1) {
        printf("Enter address: ");
        fgets(msg->address, MAX_ADDRESS, stdin);
        msg->address[strcspn(msg->address, "\n")] = 0;
        
        if (strlen(msg->address) == 0) {
            printf("Address cannot be empty!\n");
            continue;
        }
        
        break;
    }
    
    //Display available trips from shared memory
    printf("\n=== Available Trips ===\n");
    
    sem_lock(semid);
    
    if (shm->tripCount == 0) {
        printf("No trips available!\n");
        sem_unlock(semid);
        exit(1);
    }
    
    for (int i = 0; i < shm->tripCount; i++) {
        if (shm->trips[i].active) {
            printf("%d. %s - $%.2f\n", i + 1, 
                   shm->trips[i].destination, 
                   shm->trips[i].price);
        }
    }
    
    int tripChoice;
    while (1) {
        printf("Select trip number: ");
        if (scanf("%d", &tripChoice) != 1) {
            printf("Invalid input!\n");
            while (getchar() != '\n') {
                continue;
            }
            
        }
        getchar();
        
        if (tripChoice < 1 || tripChoice > shm->tripCount || 
            !shm->trips[tripChoice - 1].active) {
            printf("Invalid trip selection!\n");
            continue;
        }
        
        //Copy trip information
        strncpy(msg->destination, shm->trips[tripChoice - 1].destination, MAX_NAME - 1);
        msg->destination[MAX_NAME - 1] = '\0';
        msg->tripPrice = shm->trips[tripChoice - 1].price;
        
        break;
    }
    
    sem_unlock(semid);
    
    //Get number of people
    while (1) {
        printf("Enter number of people: ");
        if (scanf("%d", &msg->numPeople) != 1) {
            printf("Invalid input!\n");
            while (getchar() != '\n') {
                continue;
            }
            
        }
        getchar();
        
        if (msg->numPeople < 1) {
            printf("Number of people must be at least 1!\n");
            continue;
        }
        
        //Calculate total price
        msg->tripPrice *= msg->numPeople;
        
        break;
    }
    
    msg->mtype = 1;
}

int main(void) {
    ClientMessage msg;
    char choice;
    
    printf("Client (Writer) - Starting...\n");
    
    //Get shared memory
    shmid = shmget(SHM_KEY, sizeof(SharedMemory), 0666);
    if (shmid == -1) {
        perror("shmget - Please start the shared memory manager first!");
        exit(1);
    }
    
    //Attach shared memory
    shm = (SharedMemory *)shmat(shmid, NULL, 0);
    if (shm == (void *)-1) {
        perror("shmat");
        exit(1);
    }
    
    //Get semaphore
    semid = get_semaphore(SEM_KEY);
    if (semid == -1) {
        printf("Semaphore not found! Please start the shared memory manager first.\n");
        cleanup();
        exit(1);
    }
    
    //Get message queue
    msgid = msgget(MSG_KEY, 0666);
    if (msgid == -1) {
        perror("msgget - Please start the server first!");
        cleanup();
        exit(1);
    }
    
    printf("Connected to shared memory and message queue.\n");
    
    while (1) {

        //Get client data
        get_client_data(&msg);
        
        //Send message to server
        if (msgsnd(msgid, &msg, sizeof(ClientMessage) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            cleanup();
            exit(1);
        }
        
        printf("\nClient data sent successfully!\n");
        
        //Ask if more data needs to be entered
        printf("\nEnter more client data? (y/n): ");
        scanf(" %c", &choice);
        getchar();
        
        if (choice != 'y' && choice != 'Y') {
            break;
        }
    }
    
    printf("Exiting client program...\n");
    cleanup();
    
    return 0;
}
