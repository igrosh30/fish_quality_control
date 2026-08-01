//SUPERVISOR CODE
// a multiprocessing environment.
#include <iostream>
#include <unistd.h>
#include <poll.h>

using namespace std;

int main()
{
    //let's first work an example problem:

    /*
    A program that waits for keyboard input (stdin is fd 0, an fd you can poll!) 
    OR prints "tick" every 5 seconds if no input arrives. 
    Type something and it echoes it immediately; stay silent and it ticks every 5 seconds.
    */

    //each program runs with already 3 fd's!
    struct pollfd input_poll; //
    //ini the poll with the params you need -> wich fd you are trakick and how you'll track it
    
    input_poll.fd = 0; // wich to track
    input_poll.events = POLLIN; //how to track
    
    uint32_t timeout = 5000;
    int poll_res = poll(&input_poll,1,5000);
    if( poll_res < 0)// return 0 -> time expired
    {
        perror("Error in polling...");
    }
    else if (poll_res == 0)
    {   
        cout<< "timeout occured, exiting..." << endl;
    }
    else
    {
        cout<< "reading input keyboard..."<< endl;
        char read_input[256] = {};
        read(0,&read_input,256);// while read returns something different than 0 we run the while!
        
        int i = 0;
        while(read_input[i] != '\n' )
        {
            cout<<"input read: "<< read_input[i++] <<endl;
            
        }
        cout<< "where read "<< i <<" characters" << endl;
    }


    return 0;
}