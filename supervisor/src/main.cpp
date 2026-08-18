//SUPERVISOR CODE
// a multiprocessing environment.
#include <iostream>
#include <unistd.h>
#include <poll.h>
#include <unistd.h>
#include <sys/wait.h>
#include <chrono>
#include <algorithm>
#include <string>

using namespace std;

//--------TIMEOUT VAR----------------------
using clk = std::chrono::steady_clock;
const uint32_t cam_timeout = 10000;    //60m -  3600000s
const uint32_t sens_timeout = 20000;    //30m


const string sensor_path_exe  = "/home/ciimar/fish_quality_control/sensor_capture/src";
const string capture_path_exe = "/home/ciimar/fish_quality_control/image_capture/image_capture"; // where is stored the exe?

/*
shh:  ssh ciimar@ciimar.local 192.168.220.114

important documentation calls:
    man 2 execve
    man 2 poll 

    For Python code for mobdus:
        Python 3.12.7
        numpy==2.5.1
        pymodbus==3.14.0
        pyserial==3.5
*/

pid_t camera_fork()
{
    pid_t p_id = fork();
    if(p_id)
    {
        //parent process! 
        return p_id;
    }
    char* argv_cam[] = { (char*)"image_capture", nullptr };
    execv("/home/ciimar/fish_quality_control/image_capture/image_capture", argv_cam);
    // only reached if execv FAILED:
    perror("execv camera");
    _exit(127);
}

int sensor_fork(int &counting_sensor)
{
    /*
    pid_t p_id = fork();

    if(p_id)
    {
        return p_id;
    }
    char* argv_sen[] = { (char*)"python3",
                     (char*)"/home/ciimar/fish_quality_control/sensor_capture/src/I4FSensReadRawData.py",
                     nullptr };
    execv("/usr/bin/python3", argv_sen);
    // only reached if execv FAILED:
    perror("execv sensor");
    _exit(127);
    */
   counting_sensor += 1;
   return counting_sensor;
}

int main()
{
    int counting_sensor = 0;
    /*
    struct pollfd poll_fds[1];
    poll_fds[0].fd = 0;
    poll_fds[0].events = POLLIN;
    for now it's just timeouts! 
    */

    //Timers
    auto now = clk::now();
    auto next_cam = now + std::chrono::milliseconds(cam_timeout);
    auto next_sens = now + std::chrono::milliseconds(sens_timeout);

    //loop:
    while(1)
    {
        
        //computations:
        now = clk::now();                                   
        auto soonest   = std::min(next_cam, next_sens);     
        auto remaining = chrono::duration_cast<chrono::milliseconds>(soonest - now).count();
        int  timeout_ms = (remaining < 0) ? 0 : (int)remaining;   

        cout << "timeout_ms:"<< timeout_ms << endl;

        int poll_res = poll(nullptr,0,timeout_ms);
        if(poll_res< 0 ) continue;
        else if(poll_res == 0)
        {
            //time expired... perform time operations to see what fork to call! 
            //Need to check which timeout occured!
            now = clk::now();
            pid_t child_id;
            if(next_cam <= now) // now(this timestap is greater than next_cam, means next_cam fired!)
            {
                cout << "Camera process fired!" << endl;
                next_cam += std::chrono::milliseconds(cam_timeout); 
                child_id = camera_fork();
            }
            if (next_sens <= now) 
            {
                next_sens += std::chrono::milliseconds(sens_timeout);
                child_id = sensor_fork(counting_sensor);
                cout << "sensor count: "<< counting_sensor << endl;
            }
            
            int st; while (waitpid(-1, &st, WNOHANG) > 0) { }
        }
        
    }  
    return 0;
}