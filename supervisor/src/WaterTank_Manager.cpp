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

    if(serial.init(port,baud)) // se vai a baixo fudeu
    {
        std::cout<<"error initializing COM with sensors!";
        return;
    }
    tankLink.init(&serial, NodeId::HOST);
    
    comFSM.init(com_check::IDLE);
    managerFSM.init(tank_manager_state::IDLE);
    poll_tankFSM.init(poll_state::IDLE);
    //this->setup_state = tank_setup::IDLE
}

//Sends PING loooks for PONG
void WaterTank_Manager::checkLink()
{
    //we maybe wrap this in a while loop + timeout-> what we want is call once and check whetther com is working! 
    switch (comFSM.state())
    {
        case com_check::IDLE:
            this->tankLink.sendPing();
            comFSM.set_state(com_check::WAIT_PONG);
            break;
        
        case com_check::WAIT_PONG:
            //check timeout
            if(comFSM.time_in_state() >= std::chrono::seconds(COM_TIMEOUT))
            {
                if(ping_tries < MAX_TRIES)
                {
                    ping_tries++;
                    tankLink.sendPing();
                    comFSM.set_state(com_check::WAIT_PONG);
                }
                else
                {
                    //COM not working! 
                    ping_tries = 0;
                    this->com_working = false;
                    comFSM.set_state(com_check::COM_ERROR);
                }
                return; // or break? break leaves the block! so we need treturn;! 
            }
            if(tankLink.poll_pong())
            {
                ping_tries = 0;
                this->com_working = true;
                comFSM.set_state(com_check::COM_OK);
            }
            break;
    }
}


void WaterTank_Manager::update_tank_state_COM()
{
    switch (poll_tankFSM.state())
    {
        case poll_state::ASK:
            ask_tries++;
            tankLink.askTankState();
            poll_tankFSM.set_state(poll_state::WAIT_STATE);
            break;

        case poll_state::WAIT_STATE:
            if (poll_tankFSM.time_in_state() >= std::chrono::seconds(COM_TIMEOUT))
            {
                if (ask_tries < MAX_TRIES) {
                    poll_tankFSM.set_state(poll_state::ASK);
                } else {
                    ask_tries = 0;
                    poll_tankFSM.set_state(poll_state::ERROR);
                }
                return;
            }
            if (tankLink.poll_tank_state(current_tank_state)) {
                // reply received — current_tank_state updated
                ask_tries = 0;
                poll_tankFSM.set_state(poll_state::ASK);  //needs to be recursive! 
            }
            break;

        case poll_state::IDLE:
            // nothing — the caller sets ASK when it wants a fresh poll
            break;

        case poll_state::ERROR:
            // terminal until caller resets
            break;
    }
}

void WaterTank_Manager::update_state()//needs access to sensors! 
{       
    int st;
    int ret;
    switch(managerFSM.state())
    {
        case tank_manager_state::IDLE:
            /*this->close_valve();
            this->pumpOFF();NEED to check this!*/
            if(managerFSM.time_in_state() >= std::chrono::minutes(SENSOR_TIMEOUT))
            {
                poll_tankFSM.set_state(poll_state::ASK);
                update_tank_state_COM();//send straigth the message! 

                managerFSM.set_state(tank_manager_state::CHECK_TANK);
            }
            break;

        case tank_manager_state::CHECK_TANK:
            
            update_tank_state_COM();
            if(current_tank_state == tank_state::STABLE)
            {
                tankLink.sendStartCycle();
                managerFSM.set_state(tank_manager_state::WAIT);
                poll_tankFSM.set_state(poll_state::ASK);
                return;
            }
            if(managerFSM.time_in_state()>= std::chrono::minutes(FILL_TIMEOUT+DRAIN_TIMEOUT))
            {
                //timeout! report something isn't working! 
                //let's hope this doesn't go here... 
            }
            break;
        
        case tank_manager_state::WAIT:
            this->update_tank_state_COM();
            if(current_tank_state== tank_state::STABLE)
            {
                sens_pid = this->sensor_fork();
                managerFSM.set_state(tank_manager_state::CAPTURE);   
                return;
            }
            if(managerFSM.time_in_state() >= std::chrono::minutes(FILL_TIMEOUT+DRAIN_TIMEOUT))    
            {
                //error! something with the tank...
            }
            break;
            
        case tank_manager_state::CAPTURE:        
            ret = waitpid(sens_pid,&st,WNOHANG);
            if(ret == 0) return;
            if (ret == -1) return;
            managerFSM.set_state(tank_manager_state::IDLE);
            poll_tankFSM.set_state(poll_state::IDLE);
            break;
    }
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
