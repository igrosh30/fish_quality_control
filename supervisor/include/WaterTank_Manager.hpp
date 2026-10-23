#pragma once 

#include <iostream>
#include <chrono>
#include <sys/wait.h>
//#include <gpiod.h>
#include <ctime>
#include <chrono>
#include <unistd.h>
#include "TankProtocol.hpp"

using namespace std;

//--------TIMEOUT VAR---------------------//
using clk = std::chrono::steady_clock;

//--------RAW TANK STATE------------------//
enum class tank_state : uint8_t
{
    IDLE         = 0,
    WAIT_CMD     = 1,
    DRAIN_AT_TOP = 2,
    DRAIN        = 3,
    FILL         = 4,
    STABLE       = 5,
};


enum class tank_manager_state
{
    INI,
    WAIT,
    CAPTURE,//were we call fork()
    STABLE,
};

enum class tank_setup
{
    IDLE,
    WAIT_PONG,//were we call fork()
    WAIT_TANK_STABLE,
    COM_ERROR,
};

class WaterTank_Manager
{
    private:
        LinuxSerialTransport serial;
        TankProtocol tankLink;//this receives a ready transp.link - LinuxSerialTransport 
        
        tank_setup setup_state;
        clk::time_point setup_entry;
        uint8_t ping_tries = 0;

        bool com_working;
        bool waiting_response = false;

    public:
        const uint8_t SENSOR_TIMEOUT = 31; //minutes 
        const uint32_t DRAIN_TIMEOUT = 10;//minutes
        const uint32_t FILL_TIMEOUT = 3;//minutes
        
        //-----------COM PARAMS--------
        const uint32_t COM_TIMEOUT = 5;//seconds
        const uint8_t MAX_TRIES = 10;

        tank_manager_state state;
        clk::time_point state_entry;
        pid_t sens_pid;

        //WaterTank(uint8_t capture_timeout = 35);
        
        void setup(const char* port,speed_t baud);

        void update_setup();
        void update_state();

        clk::duration time_in_state() const;
        clk::duration time_in_setup() const;

        void set_next_state(tank_manager_state s );
        void set_setup_state(tank_setup s);
        pid_t sensor_fork();
};

