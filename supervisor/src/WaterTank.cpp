#include "WaterTank.hpp"


//TODO's
/*
    1- add timeouts so even if the sensors are not working, we can still perform this! ✅
    2- add the restart / reboot safety state  
    3- add a test function for the GPIOS before run the tank state update
    4- add a fuction that looks to diagnose the GPIOS everytime! maybe it can start working?... or still if we have this it can send us the error! 
    5- Check if we need to be calling always pumpOn for pumping the water- I thikn not bc this is a relay - check 


   🚨 /dev/ttyACM0 - ACM stands for the ACM modem on the USB bus. Arduino UNOs (and similar) will appear using this name.

*/


bool WaterTank::setup_env()//ASK SENS if all ok - 
{   
    switch (this->current_tank_state)
    {
        case tank_manager_state::IDLE:

            break;
    } 

   return this->current_tank_state == tank_manager_state::STABLE;  
}


void WaterTank::update_state()//needs access to sensors! 
{       
    int st;
    int ret;
    switch(this->current_tank_state)
    {
        case tank_manager_state::STABLE:
            /*this->close_valve();
            this->pumpOFF();NEED to check this!*/
            if(this->time_in_state() >= std::chrono::minutes(SENSOR_TIMEOUT))
            {
                std::cout<<"timeout! "<<std::endl;
                //send CMD_START to WaterTank 

            }
            break;

        case tank_manager_state::CAPTURE:
            //this->close_valve(); // for safety- still check if
            //this->pumpOFF(); // for safety
            
            ret = waitpid(sens_pid,&st,WNOHANG);
            if(ret == 0) return;
            if (ret == -1) return;
            set_next_state(tank_manager_state::STABLE);

            break;
    }
}

void WaterTank:: set_next_state(tank_manager_state s )
{
    if(s != current_tank_state)
    {
        current_tank_state = s;
        state_entry = clk::now();
    }   
}

clk::duration WaterTank::time_in_state() const {
    return clk::now() - state_entry;
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
        (char*)"/home/ciimar/fish_quality_control/data/pending",
        nullptr
    };
    execv("/home/ciimar/fish_quality_control/env/bin/python3", argv_sen);
    // only reached if execv FAILED:
    perror("execv sensor");
    _exit(127);
}