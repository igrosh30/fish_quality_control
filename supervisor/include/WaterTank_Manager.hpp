#pragma once 

#include <iostream>
#include <sys/wait.h>
//#include <gpiod.h>
#include <ctime>
#include <unistd.h>

#include "TankProtocol.hpp"
#include "FSM.hpp"

using namespace std;

enum class tank_manager_state
{
    IDLE,
    CHECK_TANK,
    WAIT_CYCLE_START,
    WAIT_CYCLE_DONE,
    CAPTURE,//were we call fork()
};

enum class link_setp
{
    IDLE,
    WAITING,
};

class WaterTank_Manager
{
    private:
        LinuxSerialTransport serial;
        TankProtocol tankLink;//this receives a ready transp.link - LinuxSerialTransport 
        
        
        clk::time_point setup_entry;
        uint8_t ping_tries = 0;
        uint8_t ask_tries = 0;

        
        bool waiting_response = false;

    public:
        FSM<tank_manager_state> managerFSM;
        FSM<link_setp> linkFSM;

        const uint8_t SENSOR_CAP_TIMEOUT = 45; //minutes - change for testing
        const uint32_t DRAIN_TIMEOUT = 3;//minutes
        const uint32_t FILL_TIMEOUT = 3;//minutes

        //-----LINK TIMEOUTS--------
        const uint32_t POLL_PERIOD  = 2;   // seconds between polls
        const uint32_t REPLY_WINDOW = 2;   // seconds to wait for an answer
        const uint32_t LINK_DEAD    = 10;  // seconds of silence = dead
        clk::time_point last_reply;
    
        pid_t sens_pid;
        tank_state current_tank_state = tank_state::CHECK_STABILITY;
        tank_state temp_tank_state = tank_state::CHECK_STABILITY;
        void setup(const char* port,speed_t baud);
        void update_state();
        void update_link();//running com periodically
        bool const isLinkActive();

        pid_t sensor_fork();
};

