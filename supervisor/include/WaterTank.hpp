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
        const uint32_t sens_timeout = 120000; //1890000
        bool gpio_active = false;
        const int num_sensors = 2; // change if we want more
        const int num_actuators = 2;
        gpio_ sensors[2];
        gpio_ actuators[2];
        gpio_ sensor_up;
        gpio_ sensor_down;
        gpio_ pump;
        gpio_ valve;

        tank_state current_tank_state;
        clk::time_point anchor_sens;
        pid_t sens_pid;

        int setup_gpios();
        bool setup_env();
        void read_sensors();
        void set_actuator(gpio_ &actuator, int val);
        void release_gpio();

        void update_state();
        pid_t sensor_fork();

        void pumpON();
        void pumpOFF();
        void valveON();
        void valveOFF();
};