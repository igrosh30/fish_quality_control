//SUPERVISOR CODE- a multiprocessing environment.

#include "WaterTank_Manager.hpp"
#include "CameraManager.hpp"

// void ini_functions(int code, WaterTank_Manager& tank_manager, CameraManager& cam_manager)
// {
//     if(code == 1)
//     {
//         cam_manager.camera_fork();
//         //could we check wheter the camFork worked?!- 
//     }
//     else if(code == 2)
//     {
//         tank_manager.sensor_fork();
//     }
//     else if(code == 3)
//     {
//         tank_manager.sensor_fork();
//         cam_manager.camera_fork();
//     }
// }

int main(int argc, char **argv)
{
    //./supervisor/supervisor code sens_timeout  
    uint8_t code = argc > 1 ? stoi(argv[1]) : 0; 
    
    CameraManager cam_manager;
    WaterTank_Manager tank_manager;
    std::cout<<"in maiin"<<std::endl;

    tank_manager.setup("/dev/arduino",B9600);
    cam_manager.setup();
             
    // Phase 2: normal operation
    while(1)
    {  
        tank_manager.update_link();
        tank_manager.update_state();//Internal State 

        cam_manager.update();
    }

    return 0;
}