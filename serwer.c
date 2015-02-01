// This program was written by Franciszek Jemioło - index number 346919.

#include "komunikacja.h"
#include <pthread.h>
#include <semaphore.h>


// L lists with K candidates are stored in this array.
candidateLists votes;

// In this array we will keep which commisions have sent their results.
int *commisions;

// How many lists, candidates on lists and commisions.
int commisionCount;
int listCount;
int candidatesCount;

// All voters, those who voted, bad votes.
int voters = 0;
int voted = 0;
int badVotes = 0;
int goodVotes = 0;


// In this variable we will keep which commisions were already handled.
int handledCommisions = 0;

// Variables for thread creation and handling.
pthread_t thread;
pthread_attr_t attr;

int errNum;
int bytes; 

// Locks for read/write access to global variables for threads.
pthread_rwlock_t rwlock;
int rc;
sem_t threadSemaphore;



//------------------------------------------------------------------------------

// The handles for SIGINT signal.
void signalHandler (int signum)
{   
    // Remove rwlock and semaphore so that threads will stop.
    if ((rc = pthread_rwlock_destroy (&rwlock)) != 0)
        syserr ("Error in removing pthread_rwlock\n");
    if ((rc = sem_destroy (&threadSemaphore)) != 0)
        syserr ("Error in removing threadSemaphore\n");
    // Cleaning all queues after work.
    removeQueues ();
    exit (0);
}

//------------------------------------------------------------------------------
// The thread that manages getting the result from commision.
void *commisionThread (void* data)
{
    pid_t getpid ();
    commisionMessage messageRecieved;
    blockMessage blMessage;
    candidateLists thVotes;
    int myCommision = *((int*) data);
    int rrc;
    int ii, jj;
    
    // Create arrays.
    thVotes.candidate = malloc (listCount * sizeof (int*));
    for (ii = 0; ii < listCount; ii++)
    {
        thVotes.candidate[ii] = malloc(candidatesCount * sizeof (int));
    }   
    // Get all the messages from my commision.
    if ((bytes = msgrcv (KOMISJA_QUEUE, &messageRecieved, commisionMessageSize, 
        (myCommision), 0)) <= 0)
            syserr ("Error in msgrcv server thread from komisja\n"); 
            
    for (ii = 0; ii < listCount; ii++)
    {
        for (jj = 0; jj < candidatesCount; jj++)
        {
            if ((bytes = msgrcv (KOMISJA_QUEUE, &blMessage, blockMessageSize, 
                ((long) myCommision), 0)) <= 0)
                    syserr ("Error in msgrcv server thread from komisja\n"); 
            thVotes.candidate[blMessage.mesg_data.listNum]
                [blMessage.mesg_data.candidate] = blMessage.mesg_data.votes;
        }
    } 
    
    // Now we have to lock our access to files.
    if ((rrc = pthread_rwlock_wrlock (&rwlock)) != 0)
        syserr ("Error in pthread_rwlock_wrlock\n");
    
    // Add to actual number of votes.
    for (ii = 0; ii < listCount; ii++)
    {
        for (jj = 0; jj < candidatesCount; jj++)
        {
            votes.candidate[ii][jj] += thVotes.candidate[ii][jj];
        }
    }   
    voters += messageRecieved.mesg_data.voters;
    voted += messageRecieved.mesg_data.voted;
    badVotes += messageRecieved.mesg_data.badVotes;
    goodVotes += messageRecieved.mesg_data.sum;
    handledCommisions++;
    
    // And now we release them
    if ((rrc = pthread_rwlock_unlock (&rwlock)) != 0)
        syserr ("Error in pthread_rwlock_unlock\n");
        
    // Free the arrays.
    for (ii = 0; ii < listCount; ii++)
    {
        free (thVotes.candidate[ii]);
    }
    free (thVotes.candidate);   
    free (data);        
    if ((rrc = sem_post (&threadSemaphore)) != 0)
        syserr ("Error in sem_post\n"); 
    return 0;
}

//------------------------------------------------------------------------------

void *raportThread (void* data)
{
    pid_t getpid ();
    int rrc;
    long queueKey = *((long*) data);
    raportMessage rapMsg;
    candidateLists rapVotes;
    blockMessage blMessage;
    int ii, jj;
    // Create arrays.
    rapVotes.candidate = malloc (listCount * sizeof (int*));
    for (ii = 0; ii < listCount; ii++)
    {
        rapVotes.candidate[ii] = malloc(candidatesCount * sizeof (int));
    }   
    // Have to lock our access to file with rdlock so the data be consistent.
    if ((rrc = pthread_rwlock_rdlock (&rwlock)) != 0)
        syserr ("Error in pthread_rwlock_wrlock\n");
    // Get consistent data.    
    rapMsg.mesg_data.commisions = handledCommisions;
    rapMsg.mesg_data.allCommisions = commisionCount;
    rapMsg.mesg_data.voters = voters;
    rapMsg.mesg_data.voted = voted;
    rapMsg.mesg_data.badVotes = badVotes;
    rapMsg.mesg_data.lists = listCount;
    rapMsg.mesg_data.candidates = candidatesCount; 
    for (ii = 0; ii < listCount; ii++)
    {
        for (jj = 0; jj < candidatesCount; jj++)
        {
            rapVotes.candidate[ii][jj] = votes.candidate[ii][jj];
        }
    }    
    
    // Release the lock.     
    if ((rrc = pthread_rwlock_unlock (&rwlock)) != 0)
        syserr ("Error in pthread_rwlock_unlock\n");
    
    // Send.
    rapMsg.mesg_type = queueKey;
    if ((bytes = msgsnd (RAPORT_QUEUE, &rapMsg, raportMessageSize, 0)) != 0)
        syserr ("Error in msgsnd server thread to raport\n");
    
    blMessage.mesg_type = queueKey;
    for (ii = 0; ii < listCount; ii++)
    {
        for (jj = 0; jj < candidatesCount; jj++)
        {
            blMessage.mesg_data.listNum = ii;
            blMessage.mesg_data.candidate = jj;
            blMessage.mesg_data.votes = rapVotes.candidate[ii][jj];
            if ((bytes = msgsnd (RAPORT_QUEUE, &blMessage, blockMessageSize, 0)) != 0)
                syserr ("Error in msgsnd server thread to raport\n");
        }
    }
    // Free the arrays.
    for (ii = 0; ii < listCount; ii++)
    {
        free (rapVotes.candidate[ii]);
    }
    free (rapVotes.candidate);             
    free (data);
    if ((rrc = sem_post (&threadSemaphore)) != 0)
        syserr ("Error in sem_post\n"); 
    return 0;
}

//------------------------------------------------------------------------------

int main (int argc, char** argv)
{
    if (argc == 4)
    {
        // Getting keys for Queues and their Ids.
        getKeys ();
        createQueues ();
        listCount = atoi (argv[1]);
        candidatesCount = atoi (argv[2]);
        commisionCount = atoi (argv[3]);
        
        // Initialization of locks and semaphores.
        if ((rc = pthread_rwlock_init (&rwlock, NULL)) != 0)
            syserr ("Error in pthread_rwlock_init\n");
        if ((rc = sem_init (&threadSemaphore, 0, MAX_THREADS)) != 0)
            syserr ("Error in sem_init\n");
        
        // Default set for handling signals.
        struct sigaction setup_action;
        sigset_t block_mask;
        
        sigemptyset (&block_mask);
        setup_action.sa_handler = signalHandler;
        setup_action.sa_mask = block_mask;
        setup_action.sa_flags = 0;

        if (sigaction (SIGINT, &setup_action, 0) == -1)
            syserr ("Error in sigaction\n");

        int i,j;
        
        // Create arrays.
        votes.candidate = malloc (listCount * sizeof (int*));
        for (i = 0; i < listCount; i++)
        {
            votes.candidate[i] = malloc(candidatesCount * sizeof (int));
        }           
        commisions = malloc (commisionCount * sizeof (int));
        // Clear commisions.
        for (i = 0; i < commisionCount; i++)
        {
            commisions[i] = 0;
        }
        
        // Clear candidateLists.
        
        for (i = 0; i < listCount; i++)
        {
            for (j = 0; j < candidatesCount; j++)
            {
                votes.candidate[i][j] = 0;
            }
        }
        
        // Structs for connection to server.
        connectionMessage messages;
        
        // Setting up connection from server to clients.
        confirmationMessage confirmation;
        // Type = 2 is only for server to client communication.
        confirmation.mesg_data.lists = listCount;
        confirmation.mesg_data.candidates = candidatesCount; 
        confirmation.mesg_type = RETURN_KEY;
        
        
        if ((errNum = pthread_attr_init (&attr)) != 0)
            //syserr (errNum, "Error in atrrinit\n");
            syserr ("Error in atrrinit\n");
        if ((errNum = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0)
            //syserr (errNum, "Error in setting attr detachstate\n");
            syserr ("Error in setting attr detachstate\n");



        // Going to handle all incoming messages and creating new threads to 
        // handle commisions and raports.    
        while (1)
        {
            // Handle all new incoming connections. 
            if ((bytes = msgrcv (SERWER_QUEUE, &messages, 
                connectionMessageSize, 0, 0)) <= 0)
                    syserr ("Error in msgrcv server\n");
            // Handling only one number at a time. Create new thread.
            if (messages.mesg_data.type == 0)
            // Handling commision. Sending to the thread msg type.
            {
                // Have to check if commision wasn't already handled.
                if (commisions[messages.mesg_data.number] == 0)
                {
                    if ((rc = sem_wait (&threadSemaphore)) != 0)
                        syserr ("Error in sem_wait\n");
                    // Confirm connection to client.
                    confirmation.mesg_data.number = messages.mesg_data.number;
                    confirmation.mesg_data.queueKey = 1L;
                    confirmation.mesg_type = RETURN_KEY + ((long) messages.mesg_data.number);
                    if ((bytes = msgsnd (KOMISJA_QUEUE, &confirmation, 
                        confirmationMessageSize, 0)) != 0)
                            syserr ("Error in msgsnd komisja\n");
                    
                    commisions[messages.mesg_data.number]++;        
                    // Creating komisja thread.
                    int* number = malloc (sizeof (int*));
                    *number = messages.mesg_data.number;
                    if ((errNum = pthread_create (&thread, &attr, 
                        commisionThread, (void *) number)) != 0)
                            syserr ("Error in creation of commision thread\n");
                }
                // Commision already processed!
                else
                {
                    // Send to client return communicate with no access msg.
                    // 0 means no access to server.
                    confirmation.mesg_data.number = 0;
                    confirmation.mesg_data.queueKey = ((long) messages.mesg_data.number);
                    confirmation.mesg_type = RETURN_KEY + ((long) messages.mesg_data.number);
                    if ((bytes = msgsnd (KOMISJA_QUEUE, &confirmation, 
                        confirmationMessageSize, 0)) != 0)
                            syserr ("Error in msgsnd komisja\n");
                }

            }
            else if (messages.mesg_data.type == 1)
            // Handling raport.
            {
                if ((rc = sem_wait (&threadSemaphore)) != 0)
                    syserr ("Error in sem_wait\n");
                confirmation.mesg_data.number = 0;
                confirmation.mesg_data.queueKey = ((long)messages.mesg_type);
                confirmation.mesg_type = messages.mesg_type;
                if ((bytes = msgsnd (RAPORT_QUEUE, &confirmation, 
                    confirmationMessageSize, 0)) != 0)
                        syserr ("Error in msgsnd raport\n");
                        
                // Creating raport thread.        
                int* key = malloc (sizeof (int*));
                *key = confirmation.mesg_data.queueKey;
                if ((errNum = pthread_create (&thread, &attr, 
                    raportThread, (void *) key)) != 0)
                        syserr ("Error in creation of raport thread\n");
            }
            else
            {
                syserr ("Error wrong data type recieved! %d\n", messages.mesg_data.type);
            }
                    
            
        }
        // Return -1 because we don't want to exit from our infinite loop other
        // than by SIGINT.
        return -1;
    }
    else
    {
        printf ("Wrong number of parameters, try using exactly three!\n");
    }
    return 0;
}
