// This program was written by Franciszek Jemioło - index number 346919.

#ifndef _KOMUNIKACJA_H_
#define _KOMUNIKACJA_H_
// In this header we will create message queues and get set their 
// indentificators.


#define PATH_MAX 255
// Maximum threads created.
#define MAX_THREADS 100


// The base number to identify confirmation from server.
#define RETURN_KEY 30000L

// Defines for result message.
#define MAX_DATA_SIZE 10000




#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "err.h"

// Ids for our project_id to ftok.
int SERWER_ID = 1;
int KOMISJA_ID = 2;
int RAPORT_ID = 3;

// Keys
key_t SERWER_KEY = -1;
key_t KOMISJA_KEY = -1;
key_t RAPORT_KEY = -1;

// Ids
int SERWER_QUEUE;
int KOMISJA_QUEUE;
int RAPORT_QUEUE;



//------------------------------------------------------------------------------
// Struct represents L lists with K candidates.
typedef struct
{
    int **candidate;
} candidateLists;

typedef struct
{
    int listNum;
    int candidate;
    int votes;
} candidateBlock;

//------------------------------------------------------------------------------
// Struct used to represent message data sent at connection.
// 0 - commision, 1 - raport.
typedef struct
{
    int number;
    int type;
} whoAmI;

// This struct represents messages sent at programs connection
typedef struct
{
    long mesg_type;
    whoAmI mesg_data;//[MAX_THREADS];
} connectionMessage;

//------------------------------------------------------------------------------
// Struct used to send confirmation of access to raport and commision programs.
// They contain two main values list and candidates and also queueKey assigned
// by the server to them.
typedef struct
{
    int number;
    int lists;
    int candidates;
    long queueKey;
} confirmationValues;

typedef struct
{
    long mesg_type;
    confirmationValues mesg_data;
} confirmationMessage;

//------------------------------------------------------------------------------
// Blocks of data that commision sends to server.
typedef struct
{
    long mesg_type;
    candidateBlock mesg_data;
} blockMessage;

//------------------------------------------------------------------------------
// This struct represent data hidden in commisionMessage.
typedef struct
{
    //candidateLists votes;
    int voters;
    int voted;
    int badVotes;
    int sum; 
} commisionData;

// This struct will represent messages (results) sent by commisions.
typedef struct
{
    long mesg_type;
    commisionData mesg_data;//[MAX_THREADS];
} commisionMessage;

//------------------------------------------------------------------------------

// Struct used to represent data in raport.
typedef struct
{
    int commisions;
    int allCommisions;
    int voters;
    int voted;
    int badVotes;
    int lists;
    int candidates;
    //candidateLists votes;//[MAX_THREADS];
} raportData;

// This struct will represent message sent from server to raport program.
typedef struct
{
    long mesg_type;
    raportData mesg_data;//[MAX_THREADS];
} raportMessage;  

//------------------------------------------------------------------------------

// Sizes of structures.
int connectionMessageSize = sizeof (connectionMessage) - sizeof (long);
int commisionMessageSize = sizeof (commisionMessage) - sizeof (long);
int raportMessageSize = sizeof (raportMessage) - sizeof (long);
int blockMessageSize = sizeof (blockMessage) - sizeof (long);
int confirmationMessageSize = sizeof (confirmationMessage) - sizeof (long);


// Recieving keys for our message queues, which thanks
// to ftok will hopefully be the same for every program in our project.
void getKeys ()
{
    if ((SERWER_KEY = ftok ("komunikacja.conf", SERWER_ID)) < 0)
        syserr ("Error in ftok for SERWER_KEY\n");
    if ((KOMISJA_KEY = ftok ("komunikacja.conf", KOMISJA_ID)) < 0)
        syserr ("Error in ftok for KOMISJA_KEY\n");
    if ((RAPORT_KEY = ftok ("komunikacja.conf", RAPORT_ID)) < 0)
        syserr ("Error in ftok for RAPORT_KEY\n");
    
}

// Creating and recieving ids for our queues.
void createQueues ()
{
    if ((SERWER_QUEUE = msgget (SERWER_KEY, IPC_CREAT|0666)) < 0)
        syserr ("Error in creating SERWER_QUEUE\n");
    if ((KOMISJA_QUEUE = msgget (KOMISJA_KEY, IPC_CREAT|0666)) < 0)
        syserr ("Error in creating KOMISJA_QUEUE\n");
    if ((RAPORT_QUEUE = msgget (RAPORT_KEY, IPC_CREAT|0666)) < 0)
        syserr ("Error in creating RAPORT_QUEUE\n");
}

// Get ids for our queues.
void getQueues ()
{
    if ((SERWER_QUEUE = msgget (SERWER_KEY, 0666)) < 0)
        syserr ("Error in creating SERWER_QUEUE\n");
    if ((KOMISJA_QUEUE = msgget (KOMISJA_KEY, 0666)) < 0)
        syserr ("Error in creating KOMISJA_QUEUE\n");
    if ((RAPORT_QUEUE = msgget (RAPORT_KEY, 0666)) < 0)
        syserr ("Error in creating RAPORT_QUEUE\n");
}

// Removing queues from kernel.
void removeQueues ()
{
    if (msgctl (SERWER_QUEUE, IPC_RMID, NULL) == -1)
        syserr ("Error in removing SERWER_QUEUE\n");
    if (msgctl (KOMISJA_QUEUE, IPC_RMID, NULL) == -1)
        syserr ("Error in removing KOMISJA_QUEUE\n");
    if (msgctl (RAPORT_QUEUE, IPC_RMID, NULL) == -1)
        syserr ("Error in removing RAPORT_QUEUE\n");
}


#endif
