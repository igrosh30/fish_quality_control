#pragma once
#include <unistd.h>
#include <string>
#include <iostream>
#include <ctime> 
#include <iomanip>
#include <sstream>
#include <filesystem>


//Return Error Protocol 
enum class CamResult : u_int8_t
{
    SUCCESS       = 0,
    CAMERA_INIT   = 2,    // camera couldn't open / init
    CAPTURE_FAIL  = 3,    // opened but grab/save failed
    EXEC_FAILED   = 127,  // execv itself failed (child couldn't even launch)
    KILLED        = 200,  // reserved: died by signal (supervisor sets this, see below)
};

//--------RAW TANK STATE------------------//
// ⚠️ MUST stay byte-identical with the Jetson's tank_state
enum class tank_state : uint8_t
{
    CHECK_STABILITY = 0,   // boot: read sensors, decide
    DRAIN_LEFTOVERS = 1,
    FILL            = 2,   // filling to level (shared by both phases)
    READY           = 3,   // levelled, idle, awaiting command
    DRAIN           = 4,   // full-cycle drain
};

// inline: one shared definition across translation units, no "defined but
// not used" warning (the static helpers above get one per file).
inline const char* to_string(tank_state s)
{
    switch (s)
    {
        case tank_state::CHECK_STABILITY: return "CHECK_STABILITY";
        case tank_state::DRAIN_LEFTOVERS: return "DRAIN_LEFTOVERS";
        case tank_state::FILL:            return "FILL";
        case tank_state::READY:           return "READY";
        case tank_state::DRAIN:           return "DRAIN";
        default:                          return "UNKNOWN";
    }
}

enum class PushResult : uint8_t
{
    SUCCESS             =0,
    INCOMPLETE          =1,
    TIMEOUT             =2,
    SERVER_OFF          =3,
    FOLDER_OFF          =4,
    //---CURL ERRRORS----(10-19)
    LOAD_CULR_FAILED    =10,
    CURL_INIT_FAILED    =11,
    EXEC_FAILED         =127,
    KILLED              =200,

};


static const char* cam_result_str(uint8_t code)
{
    switch (static_cast<CamResult>(code))
    {
        case CamResult::SUCCESS:      return "SUCCESS";
        case CamResult::CAMERA_INIT:  return "CAMERA_INIT";
        case CamResult::CAPTURE_FAIL: return "CAPTURE_FAIL";
        case CamResult::EXEC_FAILED:  return "EXEC_FAILED";
        case CamResult::KILLED:       return "KILLED";
        default:                      return "UNKNOWN";
    }
}
    
static const char* push_result_str(uint8_t code)
{
    switch (static_cast<PushResult>(code))
    {
        case PushResult::SUCCESS:           return "SUCCESS";
        case PushResult::INCOMPLETE:        return "INCOMPLETE";
        case PushResult::TIMEOUT:           return "TIMEOUT";
        case PushResult::SERVER_OFF:        return "SERVER_OFF";
        case PushResult::EXEC_FAILED:       return "EXEC_FAILED";
        case PushResult::KILLED:            return "KILLED";
        case PushResult::FOLDER_OFF:        return "FOLDER_OFF";
        case PushResult::LOAD_CULR_FAILED:  return "LOAD_CULR_FAILED";
        case PushResult::CURL_INIT_FAILED:  return "CURL_INIT_FAILED";
        default:                            return "UNKNOWN";
    }
}

//Live status snapshot written by the supervisor (1 Hz).
// /dev/shm is tmpfs: rewritten every second with zero flash wear.
const char* const status_json_path = "/dev/shm/fish_status.json";

//Camera Parameters:
const int num_def_cap = 2;
/*Where we will store image_captures:*/
const std::string pendig_def_path = "/home/ciimar/fish_quality_control/data/pending"; //CANNOT END IN /pending/ the / at the end will trow errors! 



