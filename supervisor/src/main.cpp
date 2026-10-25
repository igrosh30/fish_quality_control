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
void ini_functions(int code, WaterTank_Manager& tank_manager, CameraManager& cam_manager)
{
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
            tank_manager.checkLink();

        }while(!tank_manager.com_working);
    }
}

int main(int argc, char **argv)
{
    //./supervisor/supervisor code sens_timeout 
    
    uint8_t code = argc > 1 ? stoi(argv[1]) : 0; 
    //uint8_t sens_timeout = argc > 2 ? stoi(argv[2]) : 0; 
    
    //CameraManager cam_manager;
    WaterTank_Manager tank_manager;
    std::cout<<"in maiin"<<std::endl;

    tank_manager.setup("/dev/ttyACM0",B9600); //🚨 LOOK if port changes under wich sircunmstances!
    //cam_manager.setup();

    char c = 0;
    while (c != 'c')
         std::cin >> c; 

    do
    {
        tank_manager.checkLink();
    } while (tank_manager.comFSM.state() != com_check::COM_OK && 
             tank_manager.comFSM.state() != com_check::COM_ERROR); 
             

    std::cout<<"link established!"<<std::endl;
    std::cout<<"check tank state? - press y "<<std::endl;
    char a = 0;
    while (a != 'y')
         std::cin >> a; 
    tank_manager.poll_tankFSM.set_state(poll_state::ASK);
    tank_manager.update_tank_state_COM();
    tank_manager.update_tank_state_COM();
    std::cout<<"current tank state: after update("<< int(tank_manager.current_tank_state)<< std::endl;

    std::cout<<"what to check again? - press y "<<std::endl;
    char b = 0;
    while (b != 'y')
         std::cin >> b;
    std::cout<<"current tank state: after update("<< int(tank_manager.current_tank_state)<< std::endl;
    
    return 0;
    
    // Phase 2: normal operation
    while(1)
    {   
        //tank.read_sensors();
        tank_manager.update_tank_state_COM();
        tank_manager.update_state();
        //cam_manager.update();
    }
    //tank.release_gpio();
    return 0;
}