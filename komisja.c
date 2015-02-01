// This program was written by Franciszek Jemioło - index number 346919.

#include "komunikacja.h"


int main (int argc, char** argv)
{
    if (argc == 2)
    {
        getKeys ();
        getQueues ();
        // Initialize commisionNumber.
        int commisionNumber = atoi (argv[1]);
        // Votes read from input.
        candidateLists myCandidates;        
        
        // Setting up connection to server.
        connectionMessage messages;
        messages.mesg_data.number = commisionNumber;
        messages.mesg_data.type = 0L;
        messages.mesg_type = (long) commisionNumber;
        
        // Setting up connection recieved from server.
        confirmationMessage confirmation;
        
        int bytes;
        
        // Send to serwer_Queue message that we want to connect.
        if ((bytes = msgsnd (SERWER_QUEUE, &messages, connectionMessageSize, 0)) != 0)
            syserr ("Error in msgsnd komisja\n");
            
        // Recieve confirmation of connection.   
        if ((bytes = msgrcv (KOMISJA_QUEUE, &confirmation, confirmationMessageSize, 
            (commisionNumber+RETURN_KEY), 0)) <= 0)
                syserr ("Error in msgrcv komisja\n");
        // Check if recieved confirmation.
        if (confirmation.mesg_type == (RETURN_KEY+commisionNumber))
        {
            if (confirmation.mesg_data.number == commisionNumber)
            // Ok we are cleared to go!
            {
                // Initialize candidates array.
                int i,j;
                myCandidates.candidate = malloc (confirmation.mesg_data.lists * sizeof (int*));
                for (i = 0; i < confirmation.mesg_data.lists; i++)
                {
                    myCandidates.candidate[i] = malloc (confirmation.mesg_data.candidates * sizeof (int));
                }     
                // Clear candidateLists

                for (i = 0; i < confirmation.mesg_data.lists; i++)
                {
                    for (j = 0; j < confirmation.mesg_data.candidates; j++)
                    {
                        myCandidates.candidate[i][j] = 0;
                    }
                }
                
                // Read input.
                int voters, voted;
                scanf ("%d %d", &voters, &voted);
                int read = 1;
                int what;
                int list, cand, howMany;
                int sum = 0;
                int lines = 0;
                while (read)
                {
                    if ((what = scanf ("%d %d %d", &list, &cand, &howMany)) != EOF)
                    {
                        if (what == 0)
                        {
                            syserr ("Error in scanf komisja\n");
                        }
                        else
                        {
                            lines++;
                            myCandidates.candidate[list-1][cand-1] = howMany;
                            sum += howMany;
                            
                        }
                    }
                    // End of file.
                    else
                        read = 0;
                }
                
                // Send data server.
                // Creating message to send to server.
                commisionMessage comMessage;
                comMessage.mesg_data.voters = voters;
                comMessage.mesg_data.voted = voted;
                comMessage.mesg_data.badVotes = voted-sum;
                comMessage.mesg_data.sum = sum;
                comMessage.mesg_type = commisionNumber;
                // Sending data to server.
                if ((bytes = msgsnd (KOMISJA_QUEUE, &comMessage, commisionMessageSize, 0)) != 0)
                    syserr ("Error in msgsnd komisja\n");
                    
                // And now all the votes.
                blockMessage blMessage;
                blMessage.mesg_type = (long) commisionNumber;
                for (i = 0; i < confirmation.mesg_data.lists; i++)
                {
                    for (j = 0; j < confirmation.mesg_data.candidates; j++)
                    {
                        blMessage.mesg_data.listNum = i;
                        blMessage.mesg_data.candidate = j;
                        blMessage.mesg_data.votes = myCandidates.candidate[i][j];
                        if ((bytes = msgsnd (KOMISJA_QUEUE, &blMessage, blockMessageSize, 0)) != 0)
                            syserr ("Error in msgsnd komisja\n");
                    }
                }
                
                
                 
                // Write success to standard ouput.
                printf ("Przetworzonych wpisów: %d\n", lines);
                printf ("Uprawnionych do głosowania: %d\n", voters);
                printf ("Głosów ważnych: %d\n", sum);
                printf ("Głosów nieważnych: %d\n", voted-sum);
                printf ("Frekwencja w lokalu: %d%%\n", ((voted * 100)/
                    ((voters > 0) ? voters : 1)));
                
                //Delete the arrays.
                for (i = 0; i < confirmation.mesg_data.lists; i++)
                {
                    free (myCandidates.candidate[i]);
                }
                free (myCandidates.candidate);
                
            }
            else
            // Oops! Some commision with the same number has already 
            // connected to server!
            {
                printf ("Odmowa dostępu\n");
            }
        }
        else
        {
            syserr ("Error wrong communicate recieved\n");
        }
    
    }
    else
    {
        printf ("Wrong number of parameters!\n Try using only one.\n");
    }
    return 0;

}
