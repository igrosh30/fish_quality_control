#pragma once 

#include <iostream>
#include <gpiod.h>



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
}


class WaterTank
{
    private:
    
    public:
        const uint32_t sens_timeout = 1890000;     
        bool gpio_active = false;
        const int num_sensors = 2; // change if we want more
        const int num_actuators = 2;
        gpio_ sensors[num_sensors];
        gpio_ actuators[num_actuators];

        tank_state current_tank_state = tank_state::IDLE;
        clk::time_point anchor_sens;
        pid_t sens_pid;


        camera_state current_cam_state = camera_state::IDLE;

        int setup_gpio();
        void read_sensors();
        void set_actuators(gpio_ actuator, int val);
        void release_gpio();

        void update_state();
        pid_t sensor_fork();
}