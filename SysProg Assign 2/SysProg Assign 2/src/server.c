#include "common.h"

int msgid = -1;
float totalPrice = 0.0;
int recordCount = 0;
volatile sig_atomic_t running = 1;

void signal_handler(int signum) {
    if (signum == SIGINT) {
        running = 0;
        printf("\n\nReceived SIGINT (Ctrl+C)...\n");
        printf("\n=== Summary ===\n");
        printf("Total records processed: %d\n", recordCount);
        printf("Total trip price: $%.2f\n", totalPrice);
        
        //Clean up message queue
        if (msgid != -1) {
            if (msgctl(msgid, IPC_RMID, NULL) == -1) {
                perror("msgctl IPC_RMID");
            } else {
                printf("Message queue removed.\n");
            }
        }
        
        exit(0);
    }
}

void display_client(ClientMessage *msg) {
    printf("%s %s | Age: %d | Address: %s | Destination: %s | People: %d | Price: $%.2f\n",
           msg->firstName, msg->lastName, msg->age, msg->address,
           msg->destination, msg->numPeople, msg->tripPrice);
    
    totalPrice += msg->tripPrice;
    recordCount++;
}

int main(void) {
    ClientMessage msg;
    
    //Set up signal handler
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    printf("Server (Reader) - Starting...\n");
    
    //Create message queue
    msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }
    
    printf("Message queue created. Waiting for client data...\n");
    printf("Press Ctrl+C to stop and display summary.\n\n");
    
    //Wait for messages
    while (running) {
        //Receieve message timeout after checking running flag
        ssize_t result = msgrcv(msgid, &msg, sizeof(ClientMessage) - sizeof(long), 1, IPC_NOWAIT);
        
        if (result == -1) {
            if (errno == ENOMSG) {
                //If there is not a message, sleep for 100ms
                usleep(100000);
                continue;
				//If its interrupted by signal, continue
            } 
            else if (errno == EINTR) {
                continue;

            } 
            else {
                perror("msgrcv");
                break;
            }
        }
        
        //Display the received client information
        display_client(&msg);
    }
    
    //Cleanup (also done in signal handler)
    if (msgid != -1) {
        msgctl(msgid, IPC_RMID, NULL);
    }
    
    return 0;
}
