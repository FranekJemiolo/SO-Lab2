// This program was written by Franciszek Jemioło - index number 346919.

#include "komunikacja.h"


int main (int argc, char** argv)
{
    if (argc < 3)
    {
        getKeys ();
        getQueues ();
        pid_t pid = getpid ();
        
        int list = -1;
        if (argc == 2)
            list = atoi(argv[1]);
        // Setting up connection to server.
        connectionMessage messages;
        
        messages.mesg_data.number = 0;
        messages.mesg_data.type = 1;
        messages.mesg_type = ((long) pid);
        int bytes;
        
        if ((bytes = msgsnd (SERWER_QUEUE, &messages, connectionMessageSize, 0)) != 0)
            syserr ("Error in msgsnd raport\n");
        
        // Structs for communication and storing recieved data.    
        raportMessage myRaport;
        confirmationMessage confirmation;
        blockMessage blMessage;
        candidateLists myCandidates;
        
        if ((bytes = msgrcv (RAPORT_QUEUE, &confirmation, confirmationMessageSize, pid, 0)) <= 0)
            syserr ("Error in msgrcv raport\n");
        
        // Create candidate array.    
        int i, j; 
        myCandidates.candidate = malloc (confirmation.mesg_data.lists * sizeof (int*));
        for (i = 0; i < confirmation.mesg_data.lists; i++)
        {
            myCandidates.candidate[i] = malloc(confirmation.mesg_data.candidates * sizeof (int));
        }    
        
        // Read from serwer the data.    
        if ((bytes = msgrcv (RAPORT_QUEUE, &myRaport, raportMessageSize, pid, 0)) <= 0)
            syserr ("Error in msgrcv raport\n");

                   
        for (i = 0; i < confirmation.mesg_data.lists; i++)
        {
            for (j = 0; j < confirmation.mesg_data.candidates; j++)
            {
                if ((bytes = msgrcv (RAPORT_QUEUE, &blMessage, blockMessageSize, pid, 0)) <= 0)
                    syserr ("Error in msgrcv raport\n");
                myCandidates.candidate[blMessage.mesg_data.listNum]
                    [blMessage.mesg_data.candidate] = blMessage.mesg_data.votes;
            }
        }
            
            
        // Write output.
        
        printf ("Przetworzonych komisji: %d / %d\n", myRaport.mesg_data.commisions, 
            myRaport.mesg_data.allCommisions);
        printf ("Uprawnionych do głosowania: %d\n", myRaport.mesg_data.voters);
        printf ("Głosów ważnych: %d\n", myRaport.mesg_data.voted - myRaport.mesg_data.badVotes);
        printf ("Głosów nieważnych: %d\n", myRaport.mesg_data.badVotes);
        printf ("Frekwencja :  %d%%\n", 
            ((myRaport.mesg_data.voted) * 100) /
            ((myRaport.mesg_data.voters > 0) ? (myRaport.mesg_data.voters) : 1));
        int listSum;
        printf ("Wyniki poszczególnych list:\n");
        if (list == -1)
        // No parameters used in raport program execution, write every list.
        {
            for (i = 0; i < myRaport.mesg_data.lists; i++)
            {
                listSum = 0;
                for (j = 0; j < myRaport.mesg_data.candidates; j++)
                {
                    listSum += myCandidates.candidate[i][j];
                }
                
                printf ("%d %d ", i+1, listSum);
                
                for (j = 0; j < myRaport.mesg_data.candidates-1; j++)
                {
                    printf ("%d ", myCandidates.candidate[i][j]);
                }
                
                printf ("%d\n", myCandidates.candidate[i][myRaport.mesg_data.candidates-1]);
            }
        }
        else
        // Write only wanted list.
        {
            listSum = 0;
            for (j = 0; j < myRaport.mesg_data.candidates; j++)
            {
                listSum += myCandidates.candidate[list-1][j];
            }
            
            printf ("%d %d ", list, listSum);
            
            for (j = 0; j < myRaport.mesg_data.candidates-1; j++)
            {
                printf ("%d ", myCandidates.candidate[list-1][j]);
            }
            
            printf ("%d\n", myCandidates.candidate[list-1][myRaport.mesg_data.candidates-1]);
        }
        //Delete the arrays.
        for (i = 0; i < confirmation.mesg_data.lists; i++)
        {
            free (myCandidates.candidate[i]);
        }
        free (myCandidates.candidate);
    }
    else
    {
        printf ("Wrong number of parameters! Try using zero or one.\n");
    }
    return 0;
}
