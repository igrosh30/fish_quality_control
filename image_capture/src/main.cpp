#include "StereoCapture.hpp"

int main(int argc, char **argv)// **argv is the same as *argv[] ! 
{
        StereoCapture capture;  
        std::string path = argc >= 2 ? argv[1] : default_path;
        int num_img = argc >= 3 ? std::stoi(argv[2]) : default_captures;
        
        if(capture.init_camera(num_img))
        {
                int tot_captures = capture.capture_images(num_img, path);
                std::cout << tot_captures << " images where stored successfully!" << std::endl;
                capture.close_camera();
                return static_cast<int>(CamResult::SUCCESS);
        }
        else
        {
                std::cout << "Error initializing camera...";
                return static_cast<int>(CamResult::CAMERA_INIT); //error initialising camera
        }
}


        /*std::cout<<"argument count:" <<argc<<std::endl;
        std::cout<<"argument vector positions:"<<std::endl;
        std::cout<<"argv[0]"<<argv[0]<<std::endl;
        std::cout<<"argv[1]"<<argv[1]<<std::endl;
        std::cout<<"argv[2]"<<argv[2]<<std::endl;*/