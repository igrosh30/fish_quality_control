#pragma once 

#include <iostream>
#include <chrono>
#include <sys/wait.h>
#include <gpiod.h>
#include <ctime>
#include <chrono>
#include <unistd.h>

using namespace std;

//--------TIMEOUT VAR------------------
using clk = std::chrono::steady_clock;

 enum class tank_manager_state
{
    INI,
    WAIT
    CAPTURE,//were we call fork()
    STABLE,
};

class WaterTank
{
    private:
    
    public:
        const uint8_t SENSOR_TIMEOUT = 31; //minutes 
        const uint32_t DRAIN_TIMEOUT = 10;//minutes
        const uint32_t FILL_TIMEOUT = 3;//minutes
    

        //tank_state current_tank_state;
        clk::time_point state_entry;
        pid_t sens_pid;

        //WaterTank(uint8_t capture_timeout = 35);
        
        bool setup_env();

        void update_state();
        clk::duration time_in_state() const;
        void set_next_state(tank_manager_state s );
        pid_t sensor_fork();
};