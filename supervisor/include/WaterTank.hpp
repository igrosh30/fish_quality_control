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

enum class tank_state
{
    IDLE,
    DRAIN,
    FILL,
    CAPTURE,//were we call fork()
    STABLE,
};

struct gpio_
{
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    int offset;
    int flag;
    int val;
    bool enabled;
};

class WaterTank
{
    private:
    
    public:
        const uint8_t SENSOR_TIMEOUT = 5; //minutes 
        const uint32_t DRAIN_TIMEOUT = 10;//minutes
        const uint32_t FILL_TIMEOUT = 3;//minutes
        
        bool gpio_active = false;// TODO: derive from .enabled after setup_gpios, or set true when GPIOs wired

        gpio_ sensor_up;
        gpio_ sensor_down;
        gpio_ pump;
        gpio_ valve;

        tank_state current_tank_state;
        clk::time_point state_entry;
        pid_t sens_pid;

        WaterTank(uint8_t capture_timeout = 35);
        int setup_gpios();
        bool setup_env();
        void read_sensors();
        void set_actuator(gpio_ &actuator, int val);
        void release_gpio();

        void update_state();
        clk::duration time_in_state() const;
        void set_next_state(tank_state s );
        pid_t sensor_fork();

        void test_gpios();
        void pumpON();
        void pumpOFF();
        void open_valve();
        void close_valve();
};