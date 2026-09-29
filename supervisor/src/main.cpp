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

Obsolete:
tmux new -s <name> 
tmux attach -t fish 

New way is the own systemd is running the code and not we inside a terminal processor!

systemctl status fish        # full picture: running? since when? memory, CPU, last log lines
systemctl is-active fish     # one word: active / failed — fast yes-no
systemctl is-enabled fish    # enabled / disabled — will it survive a reboot?


journalctl -u fish -n 50           # last 50 lines
journalctl -u fish -f              # live tail, like watching the terminal   (Ctrl+C to exit)
journalctl -u fish -b              # only since last boot   (-b = boot)
journalctl -u fish -p err          # errors/warnings only   (-p = priority)
journalctl -u fish --since "1 hour ago"   # time-windowed


_PROCESS MANAGMENT_:
Process tree: watch -n 0.5 'pstree -p $(pgrep -x supervisor)'
check a process running with a name: pgrep -a name *not the best! 

Python Sensor run code:
/home/ciimar/fish_quality_control/env/bin/python3 \
  /home/ciimar/fish_quality_control/sensor_capture/src/I4FSensReadRawData.py \
  -c /home/ciimar/fish_quality_control/sensor_capture/config/sensors_config.json \
  -o /home/ciimar/fish_quality_control/data/sensors

*/
WaterTank tank; 
CameraManager cam_manager;

int main(int argc, char **argv)// 1 to capture at the instance 0- default
{
    uint8_t capture_atRunning = argc > 1 ? stoi(argv[1]) : 0;

    //O_CREAT - creates a file not the directory - if doens't exist open() creases
       
    cam_manager.setup();
    if(capture_atRunning == 1)
    {
        cam_manager.camera_fork();
        //could we check wheter the camFork worked?!- 
    }
    
    /*
    int err = tank.setup_gpios();
    if(err)
    {
        std::cout<<"GPIOS error initializing sensors "<<err << endl;
        Can we run without the GPIOS? -> Don't think so...
        return -1;
    }    
    */


    /*try to add this in the setup!?
    tank.anchor_sens = clk::now();

    while(tank.setup_env())
    {

    }
    */
    while(1)
    {   
        //Add a constant loop running! 
        
        //tank.read_sensors();
        cam_manager.update();
        //tank.update_state();
    
    }
    //tank.release_gpio();
    return 0;
}