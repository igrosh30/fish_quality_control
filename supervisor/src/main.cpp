//SUPERVISOR CODE- a multiprocessing environment.

#include "WaterTank_Manager.hpp"
#include "CameraManager.hpp"
#include "StatusRegistry.hpp"
#include "EventLog.hpp"

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

    //---EVENT LOG: one JSON line per FSM transition / worker exit---
    EventLog events;                 // must outlive the loop: the sinks hold &events
    events.open(events_jsonl_path);

    // one sink shared by every FSM; the FSM already says which one it is
    auto sink = [&events](const char* fsm, const char* from, const char* to, const char* reason) {
        events.transition(fsm, from, to, reason);
    };
    cam_manager.camFSM.set_sink(sink);
    tank_manager.managerFSM.set_sink(sink);
    // linkFSM deliberately has NO sink: it flips IDLE<->WAITING every 2 s by
    // design (~43k lines/day of noise). Its health is in the snapshot already.

    cam_manager.set_event_log(&events);
    tank_manager.set_event_log(&events);

    //---STATUS SNAPSHOT (read by watch/display_debug, not by the FSMs)---
    StatusRegistry status;
    const clk::time_point start = clk::now();
    long seq = 0; // lives as long as main(), so capturing it by reference is safe

    status.add("sup.ts_unix", [] { return std::time(nullptr); });
    status.add("sup.ts_local", [] {
        std::time_t now = std::time(nullptr);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        return std::string(buf);
    });
    status.add("sup.seq", [&seq] { return ++seq; });
    status.add("sup.uptime_s", [start] {
        return std::chrono::duration_cast<std::chrono::seconds>(clk::now() - start).count();
    });
    status.add("sup.pid", [] { return getpid(); });

    tank_manager.register_status(status);
    cam_manager.register_status(status);

    clk::time_point next_publish = clk::now();

    // Phase 2: normal operation
    while(1)
    {
        tank_manager.update_link();
        tank_manager.update_state();//Internal State

        cam_manager.update();

        if(clk::now() >= next_publish) // 1 Hz gate; never delays the loop itself
        {
            status.publish(status_json_path);
            next_publish = clk::now() + std::chrono::seconds(1);
        }
    }

    return 0;
}