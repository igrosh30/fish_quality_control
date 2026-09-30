#pragma once

#include <poll.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctime>
#include <unistd.h>
#include <algorithm>
#include <chrono>
#include "Config.h"
#include <cstring>
#include <chrono>
#include <signal.h>


//--------TIMEOUT VAR------------------
using clk = std::chrono::steady_clock;

enum class cam_manager_state
{
    IDLE,
    CAPTURE,// call fork()
    PUSH_IMG,
    PUSH_KILLING
};


class CameraManager
{
    private:
    /* data */
    //LogFile path
    const std::string cam_log_file =  "/home/ciimar/fish_quality_control/data/logCam.txt";
    const uint32_t cam_timeout  = 1;//hours
    const int FRAMES_REQUESTED = 1;
    
    int captures;
    bool push_finished;

    /*Cam val*/
    cam_manager_state current_state;
    pid_t pid_cap;
    pid_t pid_push;
    clk::time_point state_entry;
    int fd;

    public:
    void setup();
    
    pid_t camera_fork();
    pid_t push_fork(int num_captures);
    void update();
    void write_logStatus(int st, int fd);//returns the status child
    void write_logMsg(const char* msg, int fd);
    void process_return_captureFork(int st);
    void process_return_pushFork(int st);

    void set_next_state(cam_manager_state s);
    clk::duration time_in_state() const;
    int count_pending();//aux method to count tot captures@ini
    bool is_nightTime() const;
};







