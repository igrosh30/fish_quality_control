#pragma once

#include <string>
#include <iostream>
#include <sl/Camera.hpp>//when compilling- specify the path with -I flag! 
#include <ctime> 
#include <iomanip>
#include <sstream>
#include <filesystem>
/*
fotos s/distorções! 
videos de 10s: 10/30 frames/s:cada frame do video fornecido sem distorções.
*/

class StereoCapture
{
private:
    //Mat image;
    sl::Camera zed;
public:
    bool camera_on = false;
    sl::Mat image_left;
    sl::Mat image_right;
    /// @brief ERROR_CODE open(InitParameters init_parameters = InitParameters()) -> from library!
    /// @param path 
    /// @param  
    
    bool init_camera(int fps);//HOW COULD RESOLUTION CHANGE?!
    bool close_camera();
    int capture_images(int num, const std::string& folder_path);
};
