
/*
* FILE		    : server.c
* PROJECT	    : System Programming Assignment 2
* PROGRAMMER	: Nicholas Reilly
* FIRST VERSION	: Nov 9 2025
* DESCRIPTION	: Server that is made to take messages from message queue and display information and total price of trip.
* REFERENCING   : Deitel, P., & Deitel, H. (2016). How to Program in C and C++ (8th ed.). Deitel & Associates Inc. 
*/

//Reference the functions that are shared amongst the program.
#include "ipc_shared.h"



//Initalize varibales
int msgid = -1;
float totalPrice = 0.0;
int recordCount = 0;
volatile sig_atomic_t running = 1;

//
// FUNCTION   : signalHandler   
// DESCRIPTION: Evaluates the signal number to see if it matches the user using Ctrl+C to kill the process.
//              If so, it will clean up the message queue to prevent data from being stored.
// PARAMETERS : int sigNum, the signal that the process received. 
// RETURNS    : nothing.
//
void signal_handler(int signum) {

    //Handle SIGINT (Ctrl+C).
    if (signum == SIGINT) {
        running = 0;
        printf("\n\nReceived SIGINT (Ctrl+C)...\n");
        printf("\n=== Summary ===\n");
        printf("Total records processed: %d\n", recordCount);
        printf("Total trip price: $%.2f\n", totalPrice);
        
        //Clean up message queue before exiting.
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

//
// FUNCTION   : displayClient 
// DESCRIPTION: Displays the message that the server got from the client in the message queue.
//                    
// PARAMETERS : a ClientMessage struct.
// RETURNS    : nothing.
//
void display_client(ClientMessage *msg) {

    //Display all fields from the client message.
    printf("%s %s | Age: %d | Address: %s | Destination: %s | People: %d | Price: $%.2f\n",
           msg->firstName, msg->lastName, msg->age, msg->address,
           msg->destination, msg->numPeople, msg->tripPrice);
    
    //Update totals for summary output.
    totalPrice += msg->tripPrice;
    recordCount++;
}

//Main entry point for the server.
int main(void) {

    //Initalize the struct for the message.
    ClientMessage msg;
    
    //Set up signal handler for clean Ctrl+C termination.
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    //Set up the Ctrl+C kill.
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    printf("Server (Reader) - Starting...\n");
    
    //Create message queue using key and permissions.
    msgid = msgget(MSG_KEY, IPC_CREAT | PERMISSIONS);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }
    
    //Print lines to show queue is created and they can Ctrl+C to kill.
    printf("Message queue created. Waiting for client data...\n");
    printf("Press Ctrl+C to stop and display summary.\n\n");
    
    //Main receive loop.
    while (running) {

        //Attempt to receive a message without blocking.
        ssize_t result = msgrcv(msgid, &msg, sizeof(ClientMessage) - sizeof(long), 1, IPC_NOWAIT);
        
        if (result == -1) {
            if (errno == ENOMSG) {

                //No message available, have it sleep and check again until once arrives.
                usleep(100000);
                continue;
            } 
            else if (errno == EINTR) {

                //Receive interrupted by signal, try to get a message again.
                continue;
            } 
            else {

                //Unexpected receive failure.
                perror("msgrcv");
                break;
            }
        }
        
        //Process and print the message received.
        display_client(&msg);
    }
    
    //Final cleanup if loop exits normally.
    if (msgid != -1) {
        msgctl(msgid, IPC_RMID, NULL);
    }
    
    return 0;
}

