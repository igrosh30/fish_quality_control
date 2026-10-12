#include "WaterTank.hpp"

void init_gpios(gpio_ &gpio, int init_flag, bool is_sensor);
void aux_release_gpio(gpio_ &gpio);
int WaterTank::setup_gpios()//WaterTank
{
    this->current_tank_state = tank_state::IDLE;

    sensor_down.offset = 144;
    sensor_up.offset = 43;
    pump.offset = 106;
    valve.offset = 105;
    
    init_gpios(sensor_down,GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN,true);
    init_gpios(sensor_up,GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN,true);

    init_gpios(pump,0,false);
    init_gpios(valve,0,false);
    return 0;
}

void init_gpios(gpio_ &gpio, int init_flag, bool is_sensor)
{
    gpio.chip= gpiod_chip_open_by_name("gpiochip0");
    gpio.line= gpiod_chip_get_line(gpio.chip, gpio.offset);
    
    if(is_sensor)
        gpio.enabled = (gpiod_line_request_input_flags(gpio.line,"input",init_flag) == 0 );
    else
        gpio.enabled = (gpiod_line_request_output(gpio.line,"output",0) == 0);
}

bool WaterTank::setup_env()
{
    /*
    switch (current_tank_state)
    {
    case tank_state::IDLE:
        
        break;
    
    default:
        break;
    } */
   return true;  
}

void WaterTank::read_sensors()
{
    this->sensor_up.val     =  gpiod_line_get_value(this->sensor_up.line);
    this->sensor_down.val   = gpiod_line_get_value(this->sensor_down.line);
    if(this->sensor_down.val == -1 || this->sensor_up.val == -1)
    {
        //trow error!...
    } 
}

void WaterTank::set_actuator(gpio_ &actuator, int val)
{
    gpiod_line_set_value(actuator.line,val);     
}

void WaterTank::close_valve()
{
    this->set_actuator(valve,0);//check if 0 is the val to turn off the valve! 
}

void WaterTank::open_valve()
{
    this->set_actuator(valve,1);//check if 1 is the val to turn off the valve! 
}

void WaterTank::pumpON()
{
    this->set_actuator(pump,1);//check if 0 is the val to turn off the valve! 
}
void WaterTank::pumpOFF()
{
    this->set_actuator(pump,0);//check if 0 is the val to turn off the valve! 
}




void aux_release_gpio(gpio_ &gpio)
{
    gpiod_line_release(gpio.line);   
    gpiod_chip_close(gpio.chip);
}
void WaterTank::release_gpio()
{
    aux_release_gpio(sensor_up);
    aux_release_gpio(sensor_down);
    aux_release_gpio(pump);
    aux_release_gpio(valve);
}

void WaterTank::update_state()//needs access to sensors! 
{
    //define that sensors[0] - bottom one!
    //define that sensors[1] - upper one!
    
    switch(this->current_tank_state)
    {
        case tank_state::IDLE:

            if(clk::now()-anchor_sens >= std::chrono::milliseconds(sens_timeout))
            {
                std::cout<<"timeout..."<<endl;
                //open valve
                if(!gpio_active)
                {
                    std::cout<<"calling sensor fork...."<< endl;
                    current_tank_state = tank_state::CAPTURE;
                    sensor_fork();
                }
                else
                {
                    this->open_valve();
                    current_tank_state = tank_state::DRAIN;
                }
            }
            break;
        
        case tank_state::DRAIN:
            
            if(sensors[0].val== 0 && sensors[1].val == 0)
            {                                               ////I'm passing many arguments! need to pass now the actuators also! - CREATE A TANK class! we put there the GPIO pins def in main only opdate()
                set_actuator(actuators[0],0);
                set_actuator(actuators[1],1); //pump water!
                current_tank_state= tank_state::FILL;
            }
            break;
        
        case tank_state::FILL:
            if(sensors[0].val== 1 && sensors[1].val == 1)
            {
                set_actuator(actuators[1],0); //pump water!
                current_tank_state= tank_state::CAPTURE;
                sens_pid= sensor_fork();
            }
            break;

        case tank_state::CAPTURE:
            set_actuator(actuators[0],0); // for safety
            set_actuator(actuators[1],0); // for safety
            int st;
            int ret = waitpid(sens_pid,&st,WNOHANG);
            if(ret > 0 )
            {
                current_tank_state= tank_state::IDLE;
                anchor_sens = clk::now();
            }
            break;
    }
}


pid_t WaterTank::sensor_fork()
{
    
    pid_t p_id = fork();

    if(p_id)
        return p_id;
    
    char* argv_sen[] =
    {
        (char*)"/home/ciimar/fish_quality_control/env/bin/python3",   
        (char*)"/home/ciimar/fish_quality_control/sensor_capture/src/I4FSensReadRawData.py",
        (char*)"-c",
        (char*)"/home/ciimar/fish_quality_control/sensor_capture/config/sensors_config.json",
        (char*)"-o",
        (char*)"/home/ciimar/fish_quality_control/data/sensors",
        nullptr
    };
    execv("/home/ciimar/fish_quality_control/env/bin/python3", argv_sen);
    // only reached if execv FAILED:
    perror("execv sensor");
    _exit(127);
}
