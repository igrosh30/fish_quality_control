#include "StereoCapture.hpp"

bool StereoCapture::init_camera(int fps)
{
    //connect the object to the hardware
    
    /*before connecting the zed object we can define the parameters:
     // Set configuration parameters
        InitParameters init_params;
        init_params.camera_resolution = RESOLUTION::HD720;         // Use HD720 video mode for USB cameras
        init_params.camera_resolution = RESOLUTION::HD1200;        // Use HD1200 video mode for GMSL cameras
        init_params.camera_fps = 60;                               // Set fps at 60
        Resolucao: 2208 x 1242 
    */
    
    sl::InitParameters init_params;
    init_params.camera_resolution = sl::RESOLUTION::HD2K;

    if(zed.open(init_params) != sl::ERROR_CODE::SUCCESS)
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
            zed.retrieveImage(image_right,sl::VIEW::RIGHT);


            auto timestamp_l = zed.getTimestamp(sl::TIME_REFERENCE::IMAGE);
            auto timestamp_r = zed.getTimestamp(sl::TIME_REFERENCE::IMAGE);

            //1- /path/timestamp.png then we write!
            std::string path_l = folder_path + "/" + std::to_string(timestamp_l.getMilliseconds()); //to_string: converts to string!             
            std::string path_r = folder_path + "/" + std::to_string(timestamp_r.getMilliseconds()); //to_string: converts to string!             
            path_l += "_lef.png";
            path_r += "_right.png";
            //std::cout<<"Stored path: "<<path<<std::endl;
            if(image_left.write(sl::String(path_l.c_str())) == sl::ERROR_CODE::SUCCESS && image_right.write(sl::String(path_r.c_str())) == sl::ERROR_CODE::SUCCESS)
            {
                saved++;
            }

        }
    }

    return saved;
}

inline cv::Mat slMat2cvMat(sl::Mat& input) {
    // Mapping between MAT_TYPE and CV_TYPE
    int cv_type = -1;
    switch (input.getDataType()) {
        case sl::MAT_TYPE::F32_C1:
            cv_type = CV_32FC1;
            break;
        case sl::MAT_TYPE::F32_C2:
            cv_type = CV_32FC2;
            break;
        case sl::MAT_TYPE::F32_C3:
            cv_type = CV_32FC3;
            break;
        case sl::MAT_TYPE::F32_C4:
            cv_type = CV_32FC4;
            break;
        case sl::MAT_TYPE::U8_C1:
            cv_type = CV_8UC1;
            break;
        case sl::MAT_TYPE::U8_C2:
            cv_type = CV_8UC2;
            break;
        case sl::MAT_TYPE::U8_C3:
            cv_type = CV_8UC3;
            break;
        case sl::MAT_TYPE::U8_C4:
            cv_type = CV_8UC4;
            break;
        default:
            break;
    }

    return cv::Mat(input.getHeight(), input.getWidth(), cv_type, input.getPtr<sl::uchar1>(sl::MEM::CPU));
}

bool StereoCapture::close_camera()
{
    zed.close();
    return true;
}
