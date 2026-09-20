#include "StereoCapture.hpp"

//PASS the num_captures + path to store
int main(int argc, char **argv)// **argv is the same as *argv[] ! 
{
        StereoCapture capture;  
        int num_img = argc >= 2 ? std::stoi(argv[1]) : num_def_cap;
        std::string path = argc >= 3 ? argv[2] : pendig_def_path;
        
/*
*SHOULD VERIFY THE PATH TO THE FOLDER FIRST BEFORE RUNNING!
*I do that inside the image capture! if doesn't exist we create it!
*/
        if(capture.init_camera(num_img)) // we could pass the fps!
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