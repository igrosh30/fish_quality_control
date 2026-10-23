//SUPERVISOR CODE
// a multiprocessing environment.

#include "WaterTank_Manager.hpp"
#include "CameraManager.hpp"

/*
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


//we can pass the timeouts that we want! 
int main(int argc, char **argv)//code- 1 run cam.fork() code- 2 sensor.fork() 
{
    /*
    ./supervisor/supervisor code sens_timeout 
    */
    uint8_t code = argc > 1 ? stoi(argv[1]) : 0; 
    //uint8_t sens_timeout = argc > 2 ? stoi(argv[2]) : 0; 
    
    //WaterTank tank(sens_timeout > 5 ? sens_timeout : 35);//this value overwrites the defined one !
    CameraManager cam_manager;    
    WaterTank_Manager tank_manager;

    tank_manager.setup("/dev/ttyACM0",B9600); //🚨 LOOK if port changes under wich sircunmstances!
    //cam_manager.setup();
    if(code == 1)
    {
        cam_manager.camera_fork();
        //could we check wheter the camFork worked?!- 
    }
    else if(code == 2)
    {
        tank_manager.sensor_fork();
    }
    else if(code == 3)
    {
        tank_manager.sensor_fork();
        cam_manager.camera_fork();
    }
    else if(code == 99)//test gpios
    {
        do
        {
            tank_manager.update_setup();

        }while(!tank_manager.com_working);
        
        return 0;
    }
    
    /*
    bool is_tank_stable = false;
    while(!is_tank_stable)
    {
        //wrap this in a constant period run
        tank.read_sensors();
        is_tank_stable = tank.setup_env();
    }*/


    // Phase 2: normal operation
    while(1)
    {   
        //tank.read_sensors();
        tank_manager.update_state();
        cam_manager.update();
    }
    //tank.release_gpio();
    return 0;
}