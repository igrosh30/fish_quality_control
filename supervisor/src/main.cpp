//SUPERVISOR CODE
// a multiprocessing environment.
//need to pass the path when compiling

#include "WaterTank.hpp"
#include "CameraManager.hpp"

/*
shh:  ssh userName@userName.local 
scp -r ciimar@192.168.220.114::/home/ciimar/fish_quality_control/data/fotos_teste_v2/20260912-075629_lef.png ~/Downloads/

important documentation calls:
    man 2 execve
    man 2 poll 

    For Python code for mobdus:
        Python 3.12.7
        numpy==2.5.1
        pymodbus==3.14.0
        pyserial==3.5

Overwrite at the jetson:
*GIT _OVERWRITE_
git fetch origin
git reset --hard origin/main

tmux attach -t fish 

_PROCESS MANAGMENT_:
Process tree: watch -n 0.5 'pstree -p $(pgrep -x supervisor)'
check a process running with a name: pgrep -a name *not the best! 

*/
WaterTank tank; 
CameraManager cam_manager;

int main(int argc, char **argv)// 1 to capture at the instance 0- default
{
    uint8_t capture_atRunning = argc > 1 : stoi(argv[1]) : 0;

    //O_CREAT - creates a file not the directory - if doens't exist open() creases
    
    cam_manager.setup();
    if(capture_atRunning == 1)
        cam_manager.camera_fork();
    
    /*
    int err = tank.setup();
    if(err)
    {
        std::cout<<"error initializing sensors "<<err << "gpios"<<endl;
        std::cout<<"error initializing actuators"<< err<< "gpios"<<endl;
        std::cout<<"running without gpios "<<endl;
    }*/

    /*try to add this in the setup!?*/
    //tank.anchor_sens = clk::now();

    while(1)
    {   
        //tank.read_sensors();
        cam_manager.update();
        //tank.update_state();
    }
    //tank.release_gpio();
    return 0;
}