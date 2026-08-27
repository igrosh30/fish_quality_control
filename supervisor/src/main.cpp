//SUPERVISOR CODE
// a multiprocessing environment.
#include <unistd.h>
#include <poll.h>
#include <sys/wait.h>
#include <algorithm>
#include <string>
#include "WaterTank.cpp"

/*
shh:  ssh userName@userName.local 192.168.220.114 

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
    CAPTURE//we call fork()
};
void update_CamState(camera_state &state, clk::time_point &anchor_cam, pid_t &cam_pid);

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
    int err = tank.setup_gpio();//what can I pass here? - the array of gpios to use?!];

    if(err)
    {
        std::cout<<"error initializing sensors "<<err << "gpios"<<endl;
        std::cout<<"error initializing actuators"<< err<< "gpios"<<endl;
        std::cout<<"running without gpios "<<endl;
    }

    camera_state current_cam_state = camera_state::IDLE;
    pid_t cam_pid  = -1;

    //ini timers...
    tank.anchor_sens = clk::now(); // if i make this global the functions can directly access it!-.....
    auto anchor_cam = clk::now();

    //automation runnig - like the loop():
    while(1)
    {   
        //put this inside a constant running loop:
        tank.read_sensors();// Do I want to be always reading this? - I only call the 
        tank.update_state();
        update_CamState(current_cam_state,anchor_cam, cam_pid);
    }
    //Release the lines
    tank.release_gpio();
    return 0;
}

void update_CamState(camera_state &state, clk::time_point &anchor_cam, pid_t &cam_pid)
{
    switch(state)
    {
        case camera_state::IDLE:
            if(clk::now()- anchor_cam >= std::chrono::milliseconds(cam_timeout))
            {

                state = camera_state::CAPTURE;
                cam_pid = camera_fork();
            }
            break;
        case camera_state::CAPTURE:
            int st;
            int ret =waitpid(cam_pid,&st, WNOHANG); 
            if(ret > 0)//if -1 is error
            {
                std::cout<< "camera fork returned ok"<< endl;
                state= camera_state::IDLE;
                anchor_cam = clk::now();
            }
            break;
    }
}