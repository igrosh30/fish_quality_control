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
const int num_def_cap = 2;
/*Where we will store image_captures:*/
const std::string pendig_def_path = "/home/ciimar/fish_quality_control/data/pending"; //CANNOT END IN /pending/ the / at the end will trow errors! 



