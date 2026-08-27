#pragma once 

#include <iostream>
#include <chrono>
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
    CAPTURE//were we call fork()
};

struct gpio_
{
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    int offset;
    int flag;
    int val;
};

class WaterTank
{
    private:
    
    public:
        const uint32_t sens_timeout = 1890000;     
        bool gpio_active = false;
        const int num_sensors = 2; // change if we want more
        const int num_actuators = 2;
        gpio_ sensors[2];
        gpio_ actuators[2];

        tank_state current_tank_state = tank_state::IDLE;
        clk::time_point anchor_sens;
        pid_t sens_pid;

        int setup_gpio();
        void read_sensors();
        void set_actuator(gpio_ &actuator, int val);
        void release_gpio();

        void update_state();
        pid_t sensor_fork();
};