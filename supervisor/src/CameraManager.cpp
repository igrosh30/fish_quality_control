#include "CameraManager.hpp"

static const char* cam_result_str(uint8_t code);

void CameraManager::setup()
{
    this->fd = open(cam_log_file.c_str(),O_CREAT | O_APPEND|O_WRONLY, 0644);//cam_log_file defined Config.h
    this->pid_cap = -1;
    this->pid_push = -1;
    this->push_finished = false;

    this->captures = this->count_pending(); // let's make this read the total number of files that we have!
    std::cout<< "Ini with "<< captures<< "already to push"<<std::endl;
    this->set_next_state(cam_manager_state::IDLE);
}

void CameraManager::update()
{
    switch(this->current_state)
    {
        int st;
        int ret;
        case cam_manager_state::IDLE:            
            if(time_in_state() >= std::chrono::hours(cam_timeout)) //TIMEOUT
            {                
                if(is_nightTime())
                {
                    if(!push_finished)//Push finish has the captures side in it!
                    {
                        this->pid_push = push_fork(count_pending());
                        if(this->pid_push > 0)
                            set_next_state(cam_manager_state::PUSH_IMG);
                        else
                        {
                            set_next_state(cam_manager_state::IDLE);
                            write_logMsg("push_fork(): 'fork() error | pid_push < 0'",this->fd);
                        }
                    }
                    else
                        set_next_state(cam_manager_state::IDLE);
                    
                }
                else //DayTime
                {
                    this->pid_cap= camera_fork();
                    if(this->pid_cap > 0)
                        set_next_state(cam_manager_state::CAPTURE);
                    else
                    {
                        set_next_state(cam_manager_state::IDLE);
                        write_logMsg("camera_fork(): 'fork() error | pid_push < 0'",this->fd);
                    }
                }
            }
            break;
        case cam_manager_state::CAPTURE:
            //we can add a timeout for the CAPTURE running state...
            ret =waitpid(this->pid_cap,&st, WNOHANG); 
            if(ret == 0) return;
            if(ret == -1) return;
            
            this->write_logStatus(st,this->fd);//ALWAYS BEFORE CHANGING STATE
            this->process_return_captureFork(st);
            this->set_next_state(cam_manager_state::IDLE);
            break;
        case cam_manager_state::PUSH_IMG:
            if(time_in_state()>= std::chrono::minutes(30))
            {
                ret =waitpid(this->pid_push,&st,WNOHANG);
                if(ret==0)//pus_image haven't finished
                {

                    kill(this->pid_push, SIGTERM);
                    this->set_next_state(cam_manager_state::PUSH_KILLING);
                    this->write_logMsg("PUSH_KILL STATE... ",this->fd);
                    return;
                }
                else if(ret == -1)
                {
                    this->write_logMsg("PUSH_IMG waitpid failed: ret==-1 ",this->fd);
                    this->set_next_state(cam_manager_state::IDLE);
                    return;
                }

            }
            else
            {
                ret = waitpid(this->pid_push,&st,WNOHANG);//NEED to add a TIMEOUT on waiting the 
                if(ret == 0) return;//still runing
                if(ret == -1) return;//error - we are not treating this...
            }
            //this will run if the child returned perfectly
            this->write_logStatus(st,this->fd);
            this->process_return_pushFork(st);
            this->set_next_state(cam_manager_state::IDLE);//this->anchor_cam = clk::now();
    }
}



pid_t CameraManager::camera_fork()
{
    pid_t p_id = fork();
    if(p_id != 0)
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
    if(p_id != 0)
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

void CameraManager::process_return_captureFork(int st)
{
    this->captures +=2;//if all ok!
    push_finished= false;
}

void CameraManager::process_return_pushFork(int st)
{
    if (WIFEXITED(st))
    {
        uint8_t code = WEXITSTATUS(st);
        const char* status = push_result_str(code);

        if(strcmp(status, "SUCCESS") == 0)//all the pictures asked where stored correctly
        {
            push_finished= true;
            captures = 0;
        }
        else if(strcmp(status, "INCOMPLETE") == 0) 
        {
            //HOW can we FLAG THIS?! -> it will retry in the next hour
            push_finished= count_pending() > 0 ? false:true;
        }
        else if(strcmp(status,"SERVER_OFF") == 0)
        {
            //SERVER off - send Notification! 
            this->write_logMsg("push_fork return: 'SERVER OFF'",this->fd);
            push_finished=true;
        }
        else if( strcmp(status, "EXEC_FAILED") == 0)
        {
            this->write_logMsg("push_fork return: 'EXEC_FAILED'",this->fd);
            //SEND A PROBLEM!!!
        }   
    }
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

    if (this->current_state == cam_manager_state::CAPTURE)
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
    else if (this->current_state == cam_manager_state::PUSH_IMG)
    {
        event = "PUSH";
        if (WIFEXITED(st))
        {
            uint8_t code = WEXITSTATUS(st);
            status = push_result_str(code);
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
    if (this->current_state == cam_manager_state::PUSH_IMG)
    {
        const char* sep = "--------------------------------------------------\n";
        if (write(fd, sep, strlen(sep)) < 0) perror("write sep");
    }
}

void CameraManager::write_logMsg(const char* msg, int fd)
{
    if (fd < 0) { perror("open log"); return; }
    if(write(fd, msg, strlen(msg))<0) perror("write log");
}


int CameraManager:: count_pending()//set's tot_captures
{
    namespace fs = std::filesystem;

    std::error_code ec;
    uint8_t count = 0;

    fs::directory_iterator it(pendig_def_path, ec);
    if (ec)   // folder missing or unreadable
    {
        std::cout << "count_pending: cannot open " << pendig_def_path
                  << " (" << ec.message() << "), assuming 0" << std::endl;
        return 0;
    }

    for (const auto& entry : it)
    {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() == ".png")   // includes the dot
            count++;
    }
    return count;
}


//---FSM HELPERS---//
void CameraManager::set_next_state(cam_manager_state s) {            // single choke point for transitions
    if (s != current_state) {                       // only reset on a real change
        current_state = s;
        state_entry = clk::now();
    }
}

clk::duration CameraManager::time_in_state() const {
    return clk::now() - state_entry;
}

bool CameraManager::is_nightTime() const
{
    time_t now = time(nullptr);
    struct tm datetime = *localtime(&now);
    return datetime.tm_hour >= 21 || datetime.tm_hour <= 7; //21h to 7h: Night Time
}