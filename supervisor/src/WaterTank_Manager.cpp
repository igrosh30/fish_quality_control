#include "WaterTank_Manager.hpp"


//TODO's
/*
    1- add timeouts so even if the sensors are not working, we can still perform this! ✅
    2- add the restart / reboot safety state  
    3- add a test function for the GPIOS before run the tank state update
    4- add a fuction that looks to diagnose the GPIOS everytime! maybe it can start working?... or still if we have this it can send us the error! 
    5- Check if we need to be calling always pumpOn for pumping the water- I thikn not bc this is a relay - check 


   🚨 /dev/ttyACM0 - ACM stands for the ACM modem on the USB bus. Arduino UNOs (and similar) will appear using this name.

*/

void WaterTank_Manager::setup(const char* port,speed_t baud)
{
    this->com_working = false;
    if(serial.init(port,baud))
    {
        std::cout<<"error initializing COM with sensors!";
        return;
    }
    //now we should link the com_sensors to the protocol!
    tankLink.init(&serial, NodeId::HOST);
    this->setup_state = tank_setup::IDLE;
}

void WaterTank_Manager::update_setup()
{
    switch (setup_state)
    {
        case tank_setup::IDLE:
            this->tankLink.sendPing();
            set_setup_state(tank_setup::WAIT_PONG);
            break;
        
        case tank_setup::WAIT_PONG:
            //check timeout
            if(time_in_setup() >= std::chrono::seconds(COM_TIMEOUT))
            {
                if(ping_tries < MAX_TRIES)
                {
                    ping_tries++;
                    tankLink.sendPing();
                    set_setup_state(tank_setup::WAIT_PONG);
                }
                else
                {
                    //COM not working! 
                    this->com_working = false;
                    set_setup_state(tank_setup::COM_ERROR);
                }
                return; // or break? break leaves the block! so we need treturn;! 
            }
            if(tankLink.poll_pong())
            {
                this->com_working = true;
                tankLink.askTankState();
                set_setup_state(tank_setup::WAIT_TANK_STABLE);
            }
            break;
        default:
            break;
    }
}


void WaterTank_Manager::update_state()//needs access to sensors! 
{       
    int st;
    int ret;
    switch(this->state)
    {
        case tank_manager_state::STABLE:
            /*this->close_valve();
            this->pumpOFF();NEED to check this!*/
            if(this->time_in_state() >= std::chrono::minutes(SENSOR_TIMEOUT))
            {
                tankLink.sendStartCycle();
                this->set_next_state(state);
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

void WaterTank_Manager:: set_next_state(tank_manager_state s )
{
    if(s != state)
    {
        state = s;
        state_entry = clk::now();
    }   
}

clk::duration WaterTank_Manager::time_in_state() const {
    return clk::now() - state_entry;
}


pid_t WaterTank_Manager::sensor_fork()
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

void WaterTank_Manager::set_setup_state(tank_setup s)
{
    if (s != setup_state) {
        setup_state = s;
        setup_entry = clk::now();
    }
}
clk::duration WaterTank_Manager::time_in_setup() const {
    return clk::now() - setup_entry;
}

