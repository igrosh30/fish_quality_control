#include <sl/Camera.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <vector>

/**
 * Simple command-line argument parser to mimic Python's argparse.
 */
std::string parseArgs(int argc, char** argv) {
    std::string config_path = "./config/stereo_cams_config.json"; // Default value
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            config_path = argv[++i];
        }
    }
    return config_path;
}

int main(int argc, char** argv) {
    // Parse the output file path from command line arguments
    std::string outcfgfname = parseArgs(argc, argv);

    // Initialize the ZED camera
    sl::Camera zed;
    sl::InitParameters init_params;
    
    // Use millimeters for baseline
    init_params.coordinate_units = sl::UNIT::MILLIMETER; 

    // [FIX APPLIED HERE]
    // You must set the resolution to match the photos you are capturing.
    // Since you are capturing in max resolution, we force the SDK to HD2K.
    init_params.camera_resolution = sl::RESOLUTION::HD2K;

    if (zed.open(init_params) != sl::ERROR_CODE::SUCCESS) {
        std::cerr << "Failed to open ZED camera. Please ensure it is connected." << std::endl;
        return 1;
    }

    // Retrieve camera information and calibration parameters
    sl::CameraInformation cam_info = zed.getCameraInformation();
    sl::CalibrationParameters calib = cam_info.camera_configuration.calibration_parameters;

    // Extract Left Camera Intrinsics
    double fx_l = calib.left_cam.fx;
    double fy_l = calib.left_cam.fy;
    double cx_l = calib.left_cam.cx;
    double cy_l = calib.left_cam.cy;

    // Extract Right Camera Intrinsics
    double fx_r = calib.right_cam.fx;
    double fy_r = calib.right_cam.fy;
    double cx_r = calib.right_cam.cx;
    double cy_r = calib.right_cam.cy;

    // Get the camera baseline
    double baseline = calib.getCameraBaseline();

    // Construct the JSON structure manually
    std::string json_content = "{\n";
    
    // Construct the Left Projection Matrix (P_L)
    // This correctly hardcodes the Identity rotation
    json_content += "  \"left_P\": [\n";
    json_content += "    [" + std::to_string(fx_l) + ", 0.0, " + std::to_string(cx_l) + ", 0.0],\n";
    json_content += "    [0.0, " + std::to_string(fy_l) + ", " + std::to_string(cy_l) + ", 0.0],\n";
    json_content += "    [0.0, 0.0, 1.0, 0.0]\n";
    json_content += "  ],\n";

    // Construct the Right Projection Matrix (P_R)
    // The X translation component is correctly set to -fx_r * baseline
    json_content += "  \"right_P\": [\n";
    json_content += "    [" + std::to_string(fx_r) + ", 0.0, " + std::to_string(cx_r) + ", " + std::to_string(-fx_r * baseline) + "],\n";
    json_content += "    [0.0, " + std::to_string(fy_r) + ", " + std::to_string(cy_r) + ", 0.0],\n";
    json_content += "    [0.0, 0.0, 1.0, 0.0]\n";
    json_content += "  ]\n";
    json_content += "}";

    // Create the output directory if it does not exist
    std::filesystem::path out_path(outcfgfname);
    if (out_path.has_parent_path() && !std::filesystem::exists(out_path.parent_path())) {
        std::filesystem::create_directories(out_path.parent_path());
    }

    // Save to the JSON file
    std::ofstream out_file(outcfgfname);
    if (out_file.is_open()) {
        out_file << json_content;
        out_file.close();
        std::cout << "Successfully saved HD2K P matrices to: " << outcfgfname << std::endl;
    } else {
        std::cerr << "Failed to write to file: " << outcfgfname << std::endl;
        zed.close();
        return 1;
    }

    // Clean up
    zed.close();
    return 0;
}