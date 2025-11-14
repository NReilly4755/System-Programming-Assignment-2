//
// FILE               : shm_manager.c
// PROJECT            : SysProg Assign 2
// PROGRAMMER         : Bibi Murwared Enayat Zada
// FIRST VERSION      : 2025-11-14
// DESCRIPTION        : This program manages the shared memory for Assignment 2.
//  It creates shared memory and a semaphore, allows reading the stored trips, 
// and destroys the shared memory using the required rogue write.
//

#include "ipc_shared.h" 

// Global variables
int shmid = -1;
int semid = -1;
SharedMemory *shm = NULL;


// Function prototypes
void display_menu() ;
void create_shared_memory();
void read_shared_memory();
void kill_shared_memory();
void cleanup();
int validate_destination(char *dest);
int ask_yes_no(const char *msg);

int main() {
    int choice;

   while (1) {
    display_menu();

    if (scanf("%d", &choice) != 1) {
        printf("Invalid input! Please enter 1–4.\n");
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
                cleanup();
                printf("Exiting...\n");
                exit(0);
            default:
                printf("Invalid choice! Please select 1–4.\n");
        }
    }

    return 0;
}

// FUNCTION     : display_menu
// DESCRIPTION  : Prints the menu options.
// PARAMETERS   : None
// RETURNS      : Nothing
void display_menu() 
{
    printf("\n=== Shared Memory Manager ===\n");
    printf("1. Create shared memory\n");
    printf("2. Read shared memory\n");
    printf("3. Kill shared memory with rogue write\n");
    printf("4. Exit\n");
    printf("Enter choice: ");
}

// FUNCTION     : create_shared_memory
// DESCRIPTION  : Creates shared memory and semaphore, initializes trips.
// PARAMETERS   : None
// RETURNS      : Nothing
void create_shared_memory() {
    if (shmid != -1) {
        printf("Shared memory already exists!\n");
        return;
    }
    
    //get shared memory
    shmid = shmget(SHM_KEY, sizeof(SharedMemory), PERMISSIONS | IPC_CREAT);
    if (shmid == -1) {
        perror("Unable to create shared memory");
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
    
    // Create semaphore
    semid = create_semaphore(SEM_KEY);
    if (semid == -1) {
        shmdt(shm);
        shmctl(shmid, IPC_RMID, NULL);
        shmid = -1;
        shm = NULL;
        printf("Failed to create semaphore\n");
        return;
    }
   

    //Initialize with some default trips
    sem_lock(semid);
    shm->tripCount = 0;
    for (int i = 0; i < MAX_TRIPS; i++) {
        shm->trips[i].active = 0;
    }
    sem_unlock(semid);

    printf("Shared memory and semaphore created successfully.\n");

    // ask for user input
    if (!ask_yes_no("\nWould you like to add trips now"))
        return;

    while (1) 
    {
        if (shm->tripCount >= MAX_TRIPS) {
            printf("Maximum trips reached!\n");
            break;
        }

        Trip newTrip;

        // Destination
        while (1) {

            if (shm->tripCount >= MAX_TRIPS) {
                printf("Maximum trips reached!\n");
                break;
            }
            
            printf("\nEnter destination: ");
            fgets(newTrip.destination, MAX_NAME, stdin);
            newTrip.destination[strcspn(newTrip.destination, "\n")] = 0;

            if (strlen(newTrip.destination) == 0) {
                printf("Destination cannot be empty!\n");
                continue;
            }
            if (!validate_destination(newTrip.destination)) {
                printf("Invalid destination. Letters and spaces only.\n");
                continue;
            }
            break;
        }

        // Price
        float priceVal;
        char buf[50];
        while (1)
        {
            printf("Enter price: ");
            if (fgets(buf, sizeof(buf), stdin) == NULL)
            {
                printf("Invalid input.\n");
                continue;
            }

            // validate price
            char extra;
            if(sscanf(buf, "%f %c", &priceVal, &extra) == 1 && priceVal > 0)
            {
                newTrip.price = priceVal;
                break;
            }

            printf("Invalid price. Please enter a number (example: 50 or 12.99).\n");
        }


        newTrip.active = 1;

        sem_lock(semid);
        shm->trips[shm->tripCount] = newTrip;
        shm->tripCount++;
        sem_unlock(semid);

        printf("Trip added successfully!\n");

        // Ask if another trip should be added
        if (!ask_yes_no("Add another trip"))
            break;
    }

    //detach shared memory
    if (shmdt(shm) == -1) {
        perror("shmdt");
    }
}

// FUNCTION     : read_shared_memory
// DESCRIPTION  : Attaches and displays all trips in shared memory.
// PARAMETERS   : None
// RETURNS      : Nothing
void read_shared_memory() 
{
    // Try to get the shared memory ID
    shmid = shmget(SHM_KEY, sizeof(SharedMemory), PERMISSIONS);
    if (shmid == -1) {
        printf("Unable to connect to the shared memory segment.\n");
        return;
    }

    //reconnect to semaphore
    semid = get_semaphore(SEM_KEY);
    if (semid == -1) {
        printf("Unable to connect to semaphore.\n");
        return;
    }

    
    // Attach to shared memory
    shm = (SharedMemory *)shmat(shmid, (void *)0, 0);
    if (shm == (void *)-1) {
        printf("Unable to attach to shared memory.\n");
        return;
    }
    
    //lock semaphore before reading
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
        
    //unlock semaphore after reading
    sem_unlock(semid);

    // Detach after reading
    if (shmdt(shm) == -1) {
        printf("Unable to detach shared memory.\n");
    }

    shm = NULL;  
}

// FUNCTION     : kill_shared_memory
// DESCRIPTION  : Performs rogue write and deletes shared memory.
// PARAMETERS   : None
// RETURNS      : Nothing
void kill_shared_memory() {

    if (shmid == -1 || shm == NULL) {
        printf("Shared memory not created yet!\n");
    }

    // Attach to the shared memory 
    shm = (SharedMemory *)shmat(shmid, (void *)0, 0);
    if (shm == (void *)-1) {
        printf("Unable to attach to shared memory segment.\n");
        return;
    }

    //lock before writing
    sem_lock(semid);

    printf("\nAttempting rogue write to kill shared memory...\n");
    
    //Attempt to write to out-of-range memory address. This is for corruption in memory.
    char *rogue_ptr = (char *)shm + sizeof(SharedMemory) + 1000;
    
    printf("Writing to address: %p (out of bounds)\n", (void *)rogue_ptr);
    
    //This may cause segmentation fault or undefined behavior
    *rogue_ptr = 'X';
    
    //unlock after write
    sem_unlock(semid);

    printf("Rogue write completed (this may have corrupted memory).\n");
        
    //detach from shared memory
    if (shmdt(shm) == -1) {
        printf("Unable to detach shared memory.\n");
    }
    // cleanup shred memory    
    cleanup();
    shmid = -1;
    shm = NULL;
    semid = -1;
}

// FUNCTION     : cleanup
// DESCRIPTION  : Removes shared memory and semaphore.
// PARAMETERS   : None
// RETURNS      : Nothing
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

// FUNCTION     : validate_destination
// DESCRIPTION  : Checks if the destination has only letters/spaces.
// PARAMETERS   : char *dest
// RETURNS      : 1 valid, 0 invalid
int validate_destination(char *dest)
{
    if (strlen(dest) == 0)
        return 0;

    for (int i = 0; i < strlen(dest); i++) {
        if (!isalpha(dest[i]) && dest[i] != ' ') {
            return 0;
        }
    }

    return 1;
}

// FUNCTION     : ask_yes_no
// DESCRIPTION  : Reads a yes/no answer from the user.
// PARAMETERS   : const char *msg
// RETURNS      : 1 = yes, 0 = no
int ask_yes_no(const char *msg)
{
    char c;
    char buffer[50];

    while (1)
    {
        printf("%s (y/n):", msg);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            printf("Invalid input.\n");
            continue;
        }

        //take only the first non-newline character
        if (sscanf(buffer, " %c", &c) != 1)
        {
            printf("Invalid choice. Please type y or n.\n");
            continue;
        }

        if (c == 'y' || c == 'Y')
            return 1;
        if (c == 'n' || c == 'N')
            return 0;

        printf("Invalid choice. Please type y or n.\n");
    }
}



