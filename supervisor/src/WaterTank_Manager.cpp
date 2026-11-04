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

    if(serial.init(port,baud))// se vai a baixo fudeu
    {
        std::cout<<"error initializing COM with sensors!";
        return;
    }
    tankLink.init(&serial, NodeId::HOST);
    
    managerFSM.init(tank_manager_state::IDLE);
    linkFSM.init(link_setp::IDLE);
    
}

void WaterTank_Manager::update_link()
{
    switch(linkFSM.state())
    {
        case link_setp::IDLE:
            if(linkFSM.time_in_state() >= std::chrono::seconds(POLL_PERIOD))
            {
                //ask
                this->tankLink.askTankState();
                linkFSM.set_state(link_setp::WAITING);
            }
            break;
        
        case link_setp::WAITING:
            if(tankLink.poll_tank_state(this->current_tank_state))
            {
                last_reply = clk::now();//received reply
                linkFSM.set_state(link_setp::IDLE);
            }
            else if(linkFSM.time_in_state() >= std::chrono::seconds(REPLY_WINDOW))
            {
                //Timeout... try again
                linkFSM.set_state(link_setp::IDLE);
            }

    }
}
bool const WaterTank_Manager::isLinkActive() 
{
    return (clk::now()- last_reply ) <= std::chrono::seconds(LINK_DEAD);

}

void WaterTank_Manager::update_state()//needs access to sensors! 
{       
    int st;
    int ret;
    switch(managerFSM.state())
    {
        case tank_manager_state::IDLE:
            
            if(managerFSM.time_in_state() >= std::chrono::minutes(SENSOR_CAP_TIMEOUT))
            {
                std::cout << "[TIMEOUT] -> CHECK_TANK" << std::endl;
                managerFSM.set_state(tank_manager_state::CHECK_TANK);
            }
            break;

        case tank_manager_state::CHECK_TANK:
            //Intermideate state waiting for tank to be stable before starting the cycle! 
            if(current_tank_state == tank_state::READY)
            {
                tankLink.sendStartCycle();
                //we can add a bool to check if we received an ACK back  
                std::cout << "[CHECK_TANK] -> sendStartCycle going to WAIT..." << std::endl;
                managerFSM.set_state(tank_manager_state::WAIT_CYCLE_START);
                return;
            }
            if(managerFSM.time_in_state()>= std::chrono::minutes(FILL_TIMEOUT+DRAIN_TIMEOUT))
            {
                std::cout << "[MGR] !! tank never reached READY" << std::endl;
                managerFSM.set_state(tank_manager_state::IDLE);
            }
            break;
        
        case tank_manager_state::WAIT_CYCLE_START: //⚠️NEED to check this... 
            if(current_tank_state != tank_state::READY)
            {
                //means received the startCycle! 
                managerFSM.set_state(tank_manager_state::WAIT_CYCLE_DONE);
            }
            else if (managerFSM.time_in_state() >= std::chrono::seconds(POLL_PERIOD))//to resend the sendStart
            {
                std::cout << "[MGR] !! START_CYCLE lost, retrying" << std::endl;
                managerFSM.set_state(tank_manager_state::CHECK_TANK);
            }
            break;
        case tank_manager_state::WAIT_CYCLE_DONE:
            
            if(current_tank_state == tank_state::READY)
            {
                std::cout << "[RECEIVED TANK READY STATE] -> let's capture" << std::endl;
                sens_pid = this->sensor_fork();
                managerFSM.set_state(tank_manager_state::CAPTURE);   
                return;
            }
            if(managerFSM.time_in_state() >= std::chrono::minutes(FILL_TIMEOUT+DRAIN_TIMEOUT+2))    
            {
                //error! something with the tank...
            }
            break;
            
        case tank_manager_state::CAPTURE:        
            ret = waitpid(sens_pid,&st,WNOHANG);
            if(ret == 0) return;
            if (ret == -1) return;
            managerFSM.set_state(tank_manager_state::IDLE);

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
