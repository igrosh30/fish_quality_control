//SUPERVISOR CODE
// a multiprocessing environment.
#include <unistd.h>
#include <poll.h>
#include <sys/wait.h>
#include <algorithm>
#include <string>
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

const uint32_t cam_timeout  = 60000;
enum class camera_state
{
    IDLE,
    CAPTURE//we call fork()
};

struct camera_val
{
    camera_state state;
    pid_t pid;
    clk::time_point &anchor_cam,
};

void update_CamState(camera_val &camera);

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
    camera_val camera;
    int err = tank.setup_gpio();

    if(err)
    {
        std::cout<<"error initializing sensors "<<err << "gpios"<<endl;
        std::cout<<"error initializing actuators"<< err<< "gpios"<<endl;
        std::cout<<"running without gpios "<<endl;
    }

    camera.state = camera_state::IDLE;
    camera.pid  = -1;

    //ini timers...
    tank.anchor_sens = clk::now(); // if i make this global the functions can directly access it!-.....
    camera.anchor_cam = clk::now();

    //automation runnig - like the loop():
    while(1)
    {   
        tank.read_sensors();
        tank.update_state();
        update_CamState(camera);
    }
    //Release the lines
    tank.release_gpio();
    return 0;
}

void update_CamState(camera_val &cam)
{
    switch(cam.state)
    {
        case camera_state::IDLE:
            if(clk::now()- cam.anchor_cam >= std::chrono::milliseconds(cam_timeout))
            {
                cam.state= camera_state::CAPTURE;
                cout<<"Timeout....calling camera fork()";
                cam.pid= camera_fork();
            }
            break;
        case camera_state::CAPTURE:
            int st;
            int ret =waitpid(cam.pid,&st, WNOHANG); 
            if(ret > 0)//if -1 is error - we are stuck here! 
            {
                std::cout<< "camera fork returned ok"<< endl;
                cam.stat= camera_state::IDLE;
                cam.anchor_cam = clk::now();
            }
            
            break;
    }
}