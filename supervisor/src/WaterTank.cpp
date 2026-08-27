#include <WaterTank.hpp>

int WaterTank::setup_gpio()//WaterTank
{
    sensors[0].offset = 144; //pin 7
    sensors[1].offset = 43; //pin 33

    actuators[0].offset = 106;//pin 31 - relayA
    actuators[1].offset = 105;//pin 29 - relayB 

    int status = 0;
    for(int i = 0; i< num_sensors; i++)
    {
        sensors[i].chip = gpiod_chip_open_by_name("gpiochip0");
        sensors[i].line = gpiod_chip_get_line(sensors[i].chip, sensors[i].offset);
        
        actuators[i].chip = gpiod_chip_open_by_name("gpiochip0");
        actuators[i].line = gpiod_chip_get_line(actuators[i].chip, actuators[i].offset);
        
        if(gpiod_line_request_input_flags(sensors[i].line,"input",GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN)) // GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN arcodede for both sensors!
        {        
            std::cout << "eerror requesting gpio.... RUNING WITH TIMEOUTS!"<<endl;
            status++;
        } 
        //Actuators
        if(gpiod_line_request_output(actuators[i].line,"output", 0))
        {
            std::cout << "eerror requesting gpio.... RUNING WITH TIMEOUTS!"<<endl;
            status += 10;
        } //further check what value need to relays... 0/1 to activate?!!!!!
    }
    if(!status) gpio_active = true;
    return status;
}

void WaterTank::read_sensors()
{
    for(int i = 0; i< this->num_sensors; i++)
    {
        int val = gpiod_line_get_value(this->sensors[i].line);
        if(val == -1) return;
        this->sensors[i].val = val;    
    }
}

void WaterTank::set_actuator(gpio_ &actuator, int val)
{
    gpiod_line_set_value(actuator.line,val);     
}




void WaterTank::release_gpio()
{
    for(int i = 0; i< 2; i++)
    {
        gpiod_line_release(this->sensors[i].line);
        gpiod_line_release(this->actuators[i].line);
        gpiod_chip_close(this->sensors[i].chip);
        gpiod_chip_close(this->actuators[i].chip);
    }
}



void WaterTank::update_state()//needs access to sensors! 
{
    std::cout<<"current tank state:"<<this->current_tank_state<<endl;
    //define that sensors[0] - bottom one!
    //define that sensors[1] - upper one!
    switch(this->current_tank_state)
    {
        case tank_state::IDLE:

            if(clk::now()-anchor_sens >= std::chrono::milliseconds(sens_timeout))
            {
                //open valve
                if(!gpio_active)
                {
                    current_tank_state = tank_state::CAPTURE;
                    sensor_fork();
                }
                else
                {
                    set_actuator(actuators[0],1); //open valve
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
    {
        //parent process received p_id of the child so it's != 0 
        return p_id;
    }
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
