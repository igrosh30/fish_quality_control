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


//--------TIMEOUT VAR------------------
using clk = std::chrono::steady_clock;

enum class camera_state
{
    IDLE,
    CAPTURE,// call fork()
    PUSH_IMG
};


class CameraManager
{
    private:
    /* data */
    //LogFile path
    const std::string cam_log_file =  "/home/ciimar/fish_quality_control/data/logCam.txt";
    const uint32_t cam_timeout  = 3600000;
    const int FRAMES_REQUESTED = 1;
    
    uint8_t captures;
    bool havePushed;

    /*Cam val*/
    camera_state current_cam_state;
    pid_t pid_cap;
    pid_t pid_push;
    clk::time_point anchor_cam;
    int fd;

    public:
    void setup();
    pid_t camera_fork();
    pid_t push_fork(int num_captures);
    void update();
    void write_logStatus(int st, int fd);
};







