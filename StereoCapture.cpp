#include "StereoCapture.h"

bool StereoCapture::init_camera(int fps)
{
    //connect the object to the hardware
    
    /*before connecting the zed object we can define the parameters:
     // Set configuration parameters
         InitParameters init_params;
         init_params.camera_resolution = RESOLUTION::HD720;         // Use HD720 video mode for USB cameras
         init_params.camera_resolution = RESOLUTION::HD1200;        // Use HD1200 video mode for GMSL cameras
         init_params.camera_fps = 60;                               // Set fps at 60
    */

    if(zed.open() != sl::ERROR_CODE::SUCCESS)
    {
        std::cout << "Error opening camerra...exit program " << std::endl;
        camera_on = false;
    }
    else camera_on = true;
    
    return camera_on;
}

int StereoCapture::capture_images(int num, const std::string& folder_path)
{
    int saved = 0;
    if(!camera_on || num <= 0)  return -1;    
    //verify if folder_path exists ?
    
    //let's grab a image!
    for(int i = 0; i<num; i++)
    {
        if(zed.grab() == sl::ERROR_CODE::SUCCESS)
        {
            //zed.retrieveImage(image_right,VIEW::RIGHT);
            zed.retrieveImage(image_left,sl::VIEW::LEFT);//left image only
            auto timestamp = zed.getTimestamp(sl::TIME_REFERENCE::IMAGE);

            //1- /path/timestamp.png then we write!
            std::string path = folder_path + "/" + std::to_string(timestamp.getMilliseconds()); //to_string: converts to string!             
            path += ".png";
            std::cout<<"Stored path: "<<path<<std::endl;
            if(image_left.write(sl::String(path.c_str())) == sl::ERROR_CODE::SUCCESS)
            {
                saved++;
            }
        }
    }

    return saved;
}

bool StereoCapture::close_camera()
{
    zed.close();
    return true;
}
