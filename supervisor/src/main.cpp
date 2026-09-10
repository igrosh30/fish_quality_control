//SUPERVISOR CODE
// a multiprocessing environment.
#include "Config.h"//need to pass the path when compiling
#include <poll.h>
#include <sys/wait.h>
#include <algorithm>
#include "WaterTank.cpp"
#include <fcntl.h>

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
const int FRAMES_REQUESTED = 1;
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
    int fd;
};

void update_CamState(camera_values &camera);
void write_logStatus(int st, int fd);
static const char* cam_result_str(uint8_t code);

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

    int fd = open(cam_log_file.c_str(),O_CREAT | O_APPEND|O_WRONLY, 0644);//cam_log_file defined Config.h

    int err = tank.setup_gpio();
    if(err)
    {
        std::cout<<"error initializing sensors "<<err << "gpios"<<endl;
        std::cout<<"error initializing actuators"<< err<< "gpios"<<endl;
        std::cout<<"running without gpios "<<endl;
    }

    camera_values cam = { camera_state::IDLE, -1, clk::now(),fd};
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
            write_logStatus(st,cam.fd);
            //reset always
            if(ret > 0)
            {
                cam.state = camera_state::IDLE;
                cam.anchor_cam = clk::now();
            }
            break;
    }
}

void write_logStatus(int st, int fd)   // by value, no &
{
    if (fd < 0)
    {
        std::cout << "error opening the file" << endl;
        return;
    }

    std::time_t now = std::time(nullptr);
    char timebuf[32];
    std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

    const char* status = "UNKNOWN";
    int photos = 0;
    char notes[64] = "";

    if (WIFEXITED(st))
    {
        uint8_t code = WEXITSTATUS(st);
        status = cam_result_str(code);
        if (code == static_cast<uint8_t>(CamResult::SUCCESS))
            photos = FRAMES_REQUESTED;
    }
    else if (WIFSIGNALED(st))
    {
        status = "KILLED";
        snprintf(notes, sizeof(notes), "signal=%d", WTERMSIG(st));
    }

    char buf[128];
    int len = snprintf(buf, sizeof(buf), "%s | %d | %s | %s\n",
                       timebuf, photos, status, notes);
    if (len > (int)sizeof(buf)) len = sizeof(buf);
    ssize_t n = write(fd, buf, len);
    if (n < 0) perror("write log");
}

static const char* cam_result_str(uint8_t code)
{
    switch (static_cast<CamResult>(code))
    {
        case CamResult::SUCCESS:      return "SUCCESS";
        case CamResult::CAMERA_INIT:  return "CAMERA_INIT";
        case CamResult::CAPTURE_FAIL: return "CAPTURE_FAIL";
        case CamResult::EXEC_FAILED:  return "EXEC_FAILED";
        case CamResult::KILLED:       return "KILLED";
        default:                      return "UNKNOWN";
    }
}