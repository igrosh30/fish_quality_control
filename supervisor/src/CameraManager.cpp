#include "CameraManager.hpp"

/*
camera_values cam = { camera_state::IDLE, -1, clk::now(),fd};
*/
void CameraManager::setup()
{
    this->fd = open(cam_log_file.c_str(),O_CREAT | O_APPEND|O_WRONLY, 0644);//cam_log_file defined Config.h
    this->current_cam_state= camera_state::IDLE;
    this->pid_cap = -1;
    this->pid_push = -1;
    this->anchor_cam = clk::now();
    this->captures = 0;
    this->havePushed = false;
}

void CameraManager::update()
{
    switch(this->current_cam_state)
    {
        case camera_state::IDLE:
            
            if(clk::now()- this->anchor_cam >= std::chrono::milliseconds(cam_timeout) )
            {
                time_t timestamp = time(&timestamp);
                struct tm datetime = *localtime(&timestamp);
                if(datetime.tm_hour>= 17 || datetime.tm_hour <= 8)
                {
                    if(!havePushed)
                    {
                        this->current_cam_state= camera_state::PUSH_IMG;
                        this->pid_push = push_fork(captures);// I can pass the total 
                    }
                    else//nothing to push
                        this->anchor_cam = clk::now();
                    
                }
                else
                {
                    this->current_cam_state= camera_state::CAPTURE;
                    this->pid_cap= camera_fork();
                }
            }
            break;
        case camera_state::CAPTURE:
            int st;
            int ret =waitpid(this->pid_cap,&st, WNOHANG); 
            if(ret == 0) return; if(ret == -1) return; 
            
            write_logStatus(st,this->fd);//ALWAYS BEFORE CHANGING STATE
            
            havePushed= false;
            this->current_cam_state = camera_state::IDLE;
            this->anchor_cam = clk::now();
            break;
        case camera_state::PUSH_IMG:
            /*
            if it's time to start taking fotos again -> Manager needs to signal to the push()
            */
            int st;
            int ret = waitpid(this->pid_push,&st,WNOHANG);
            if(ret == 0) return; if(ret == -1) return;

            write_logStatus(st,this->fd);
            /*
            Reset parameters
            */
            havePushed= true;
            this->current_cam_state= camera_state::IDLE;
            this->anchor_cam = clk::now();
    }
}



pid_t CameraManager::camera_fork()
{
    pid_t p_id = fork();
    if(p_id)
        return p_id;
    
    char* argv_cam[] = { //THE PATH IS HARDCODED TO NAME MAHCINE!
        (char*)"image_capture",//binary to run
        (char*) "1",
        //(char*) pendig_def_path,//change this to folder pending! 
        nullptr
    };
    execv("/home/ciimar/fish_quality_control/image_capture/image_capture", argv_cam);
    // only reached if execv FAILED:
    perror("execv camera");
    _exit(127);
}

pid_t CameraManager::push_fork(int num_captures)
{
    pid_t p_id = fork();
    if(p_id)
        return p_id;

    std::string n = std::to_string(num_captures);
    char* argv_push[] = { //THE PATH IS HARDCODED TO NAME MAHCINE!
        (char*)"push_captures",//binary to run
        (char*)n.c_str(),
        /*only if we want to pass the path 
        (char*) "/home/ciimar/fish_quality_control/data/fotos_teste_v2",
        */
        nullptr
    };
    execv("/home/ciimar/fish_quality_control/push_captures/push_captures", argv_push);
    perror("execv push");
    _exit(127);
}

void CameraManager::write_logStatus(int st, int fd)
{
    if (fd < 0) { perror("open log"); return; }

    std::time_t now = std::time(nullptr);
    char timebuf[32];
    std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

    const char* event  = "UNKNOWN";
    const char* status = "UNKNOWN";
    int number = 0;                 // capture: photos taken | push: images that FAILED
    char notes[64] = "";

    // shared mechanism: killed-by-signal is identical for both workers
    if (WIFSIGNALED(st))
    {
        status = "KILLED";
        snprintf(notes, sizeof(notes), "signal=%d", WTERMSIG(st));
    }

    if (this->current_cam_state == camera_state::CAPTURE)
    {
        event = "CAPTURE";
        if (WIFEXITED(st))
        {
            uint8_t code = WEXITSTATUS(st);
            status = cam_result_str(code);
            if (code == static_cast<uint8_t>(CamResult::SUCCESS))
                number = FRAMES_REQUESTED;
        }
    }
    else if (this->current_cam_state == camera_state::PUSH_IMG)
    {
        event = "PUSH";
        if (WIFEXITED(st))
        {
            uint8_t code = WEXITSTATUS(st);
            if (code == 127)              // execv sentinel, not an error count
                status = "EXEC_FAILED";
            else
            {
                number = code;            // number of images that failed to push
                status = code ? "PUSH_ERRORS" : "OK";
            }
        }
    }

    // shared tail: one format, one write, for both events
    char buf[160];
    int len = snprintf(buf, sizeof(buf), "%s | %s | %d | %s | %s\n",
                       timebuf, event, number, status, notes);
    if (len < 0) return;
    if (len >= (int)sizeof(buf)) len = sizeof(buf) - 1;
    if (write(fd, buf, len) < 0) perror("write log");

    // day separator: the push is the last event of the day
    if (this->current_cam_state == camera_state::PUSH_IMG)
    {
        const char* sep = "--------------------------------------------------\n";
        if (write(fd, sep, strlen(sep)) < 0) perror("write sep");
    }
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
