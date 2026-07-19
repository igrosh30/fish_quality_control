#include "StereoCapture.h"


const std::string default_path = "/home";
const int default_captures = 30;

int main(int argc, char **argv)// **argv is the same as *argv[] ! 
{
        StereoCapture capture;  
        std::string path = argc <= 2 ? argv[1] : default_path;
        int num_img = argc >= 3 ? std::stoi(argv[2]) : default_captures;
        
        if(capture.init_camera(num_img))
        {
                int tot_captures = capture.capture_images(num_img, path);
                std::cout << tot_captures << " images where stored successfully" << std::endl;
                capture.close_camera();
                return 0;
        }
        else
        {
                std::cout << "Error initializing camera...";
                return -2; //error initialising camera
        }
}
