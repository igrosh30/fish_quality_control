//SUPERVISOR CODE
// a multiprocessing environment.
#include "Config.h"//need to pass the path when compiling
#include <poll.h>
#include <sys/wait.h>
#include <algorithm>
#include "WaterTank.cpp"

/*
shh:  ssh userName@userName.local 

important documentation calls:
    man 2 execve
    man 2 poll 

    For Python code for mobdus:
        Python 3.12.7
        numpy==2.5.1
        pymodbus==3.14.0
        pyserial==3.5

Overwrite at the jetson:
git fetch origin
git reset --hard origin/main

Amonia turn off!

tmux attach -t fish 

*/
WaterTank tank; 

const uint32_t cam_timeout  = 3600000;
enum class camera_state
{
    IDLE,
    CAPTURE// call fork()
};

struct camera_values
{
    camera_state state;
    pid_t pid;
    clk::time_point anchor_cam;
};

void update_CamState(camera_values &camera);

pid_t camera_fork()
{
    pid_t p_id = fork();
    if(p_id)
    {
        //parent process! 
        return p_id;
    }
    char* argv_cam[] = { //THE PATH IS HARDCODED TO NAME MAHCINE!
        (char*)"image_capture",//binary to run
        (char*) "/home/ciimar/fish_quality_control/data/fotos_teste_v2",
        (char*) "1",
        nullptr
    };
    execv("/home/ciimar/fish_quality_control/image_capture/image_capture", argv_cam);
    // only reached if execv FAILED:
    perror("execv camera");
    _exit(127);
}

int main()
{
    int err = tank.setup_gpio();

    if(err)
    {
        std::cout<<"error initializing sensors "<<err << "gpios"<<endl;
        std::cout<<"error initializing actuators"<< err<< "gpios"<<endl;
        std::cout<<"running without gpios "<<endl;
    }

    camera_values cam = { camera_state::IDLE, -1, clk::now()};
    tank.anchor_sens = clk::now(); // if i make this global the functions can directly access it!-.....

    //automation runnig - like the loop():
    while(1)
    {   
        //tank.read_sensors();
        update_CamState(cam);
        tank.update_state();
    }
    //Release the table lines
    tank.release_gpio();
    return 0;
}

void update_CamState(camera_values &cam)
{
    switch(cam.state)
    {
        case camera_state::IDLE:
            if(clk::now()- cam.anchor_cam >= std::chrono::milliseconds(cam_timeout))
            {
                std::cout<<"Timeout....calling camera fork()"<< endl;
                cam.state= camera_state::CAPTURE;
                cam.pid= camera_fork();
            }
            break;
        case camera_state::CAPTURE:
            int st;
            int ret =waitpid(cam.pid,&st, WNOHANG); 
            if(ret == 0) return;
            if(ret == -1) return; // para quê vereficar!? 
            //store info
            //writeStatus(st);
            //reset always
            if(ret > 0)
            {
                cam.state = camera_state::IDLE;
                cam.anchor_cam = clk::now();
            }
            break;
    }
}

/*
void write_logStatus(int &st) //write to a log file
{
    if (WIFEXITED(st))
    {
        uint8_t code = WEXITSTATUS(st);      
        //we can just write to the file the code result! 
        if(code == static_cast<int>(CamResult::SUCCESS))
        {
                    
        }
        else if(code == code == static_cast<int>(CamResult::CAMERA_INIT))
        {
                
        }
            
    }
    else if (WIFSIGNALED(st))
    {
        int sig = WTERMSIG(st);                // crashed/killed, no exit code exists        
    }
}*/