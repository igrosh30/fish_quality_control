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
    WAIT,
    CAPTURE,//were we call fork()
};

enum class poll_state
{
    IDLE,       // not polling — waiting for someone to trigger
    ASK,        // send the askTankState request
    WAIT_STATE, // waiting for the reply
    ERROR,      // MCU not responding after retries
};

enum class com_check
{
    IDLE,
    WAIT_PONG,//were we call fork()
    COM_OK,
    COM_ERROR,
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
        FSM<com_check> comFSM;
        FSM<tank_manager_state> managerFSM;
        FSM<poll_state> poll_tankFSM;


        const uint8_t SENSOR_TIMEOUT = 31; //minutes 
        const uint32_t DRAIN_TIMEOUT = 10;//minutes
        const uint32_t FILL_TIMEOUT = 3;//minutes
        
        //-----------COM PARAMS--------
        const uint32_t COM_TIMEOUT = 2;//seconds
        const uint8_t MAX_TRIES = 10;
        bool com_working;
    
        pid_t sens_pid;
        tank_state current_tank_state = tank_state::IDLE;
        
        void setup(const char* port,speed_t baud);
        void checkLink();//checks wheter com is working! - the only func changing com_working!
        void update_state();

        void update_tank_state_COM();

        pid_t sensor_fork();
};

