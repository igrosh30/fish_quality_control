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


//Camera Parameters:
const std::string default_path = "/home/ciimar/data/fotos_default";
const int default_captures = 5;

//LogFile path
const std::string cam_log_file =  "/home/ciimar/fish_quality_control/data/logCam.txt";