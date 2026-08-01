//SUPERVISOR CODE
// a multiprocessing environment.
#include <iostream>
#include <unistd.h>
#include <poll.h>
#include <unistd.h>
#include <sys/wait.h>



using namespace std;
const uint32_t cam_timeout = 3600000;    //60m
const uint32_t sen_timeout = 1800000;    //30m

/*
important documentation calls:
    shh:  ssh ciimar@ciimar.local   
    
    man 2 execve
    man 2 poll 

    For Python code:
        Python 3.12.7
        numpy==2.5.1
        pymodbus==3.14.0
        pyserial==3.5
*/

void manage_camera_poll(int val)
{
    if(val < 0) // return 0 -> time expired
    {
        perror("Error in polling...");
    }
    else if (val == 0) //TIMEOUT
    {   
        cout<< "timeout occured, calling fork()..." << endl;
        pid_t cam_pid = fork();
        if(cam_pid < 0)
        {
            perror("Error forking...");
        }
        else if(cam_pid == 0) //child process
        {
            //call exc!
        }
    }
    else
    {
        //Modem fd Call   
    }
}

int main()
{

    struct pollfd poll_fds[2];

    //Camera
    poll_fds[0].fd = 0;
    poll_fds[0].events = POLLIN;

    poll_fds[1].fd = 0;
    poll_fds[1].events = POLLIN;    
    
    //poll says when a file descriptor is ready for reading/writing/...

    //loop:
    while(1)
    {
        manage_camera_poll(poll(&poll_fds[0],1,-1));

    }


    return 0;
}