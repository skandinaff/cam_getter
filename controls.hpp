#ifndef CONTROLS_HPP
#define CONTROLS_HPP

#include <fstream>
#include <iostream>
#include <filesystem>
#include <jsoncpp/json/json.h>

#include "cam.h"
#include "capabilities.h"

// v4l2-ctl -d /dev/video1 -l // command to check cameras controls

#define CUSTOM_CID_EXPOSURE_DYNAMIC_FRAMERATE 0x009a0903

class CamCtrl
{
public:
    struct CameraControls {
        int brightness = 0;
        int contrast = 32;
        int saturation = 64;
        int hue = 0;
        int autoWhiteBalance = 1;
        int gamma = 100;
        int gain = 0;
        int powerLineFrequency = 1;
        int whiteBalanceTemperature = 4600;
        int sharpness = 3;
        int backlightCompensation = 1;
        int exposureAuto = 3;
        int exposureTimeAbsolute = 157;
        int exposure_dynamic_framerate = 0;
    };
    bool create_cam_config_file(const std::string& filename, const CameraControls& controls);
    bool read_cam_config_file(const std::string& filename, CameraControls& controls);
    bool load_cam_config(const std::string& filename, CameraControls& controls);
    void set_camera_control(ImageGetter * g, __u32 controlId, __s32 value);
    void set_camera_controls(ImageGetter * g, const CameraControls& controls);
    void check_camera_capabilities(ImageGetter * g, const char * device);
    bool is_control_supported(ImageGetter * g, __u32 controlId);
};

inline std::string getV4l2CtlOutput() {
    std::string command = "v4l2-ctl -d " + std::string(video_device) + " -L";

    std::stringstream output;
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe) {
        char buffer[128];
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != nullptr) {
                output << buffer;
            }
        }
        pclose(pipe);
    }

    return output.str();
}

inline bool addBoilerplateToJsonFile(const std::string& filePath) {

    std::ifstream fileIn(filePath);  // Open the existing JSON file
    if (!fileIn.is_open()) {
        std::cerr << "Failed to open the JSON file." << std::endl;
        return false;
    }

    std::string existingJsonData((std::istreambuf_iterator<char>(fileIn)), std::istreambuf_iterator<char>());

    fileIn.close();  // Close the file

    std::ofstream fileOut(filePath);  // Open the JSON file for writing
    if (!fileOut.is_open()) {
        std::cerr << "Failed to open the JSON file for writing." << std::endl;
        return false;
    }

    std::string boilerplateText = "/* Avaliable settinngs of: " 
                                    + (std::string)video_device 
                                    + "\n WARNING: The order of keys is mixed"
                                    + "\n" + getV4l2CtlOutput() 
                                    + "*/\n";
    fileOut << boilerplateText << existingJsonData;  // Write the boilerplate text and the existing JSON data to the file

    fileOut.close();  // Close the file

    std::cout << "Boilerplate text added successfully." << std::endl;
    return true;
}


inline bool CamCtrl::create_cam_config_file(const std::string& filename, const CameraControls& controls) {
    Json::Value root;
    root["brightness"] = controls.brightness;
    root["contrast"] = controls.contrast;
    root["saturation"] = controls.saturation;
    root["hue"] = controls.hue;
    root["white_balance_automatic"] = controls.autoWhiteBalance;
    root["gamma"] = controls.gamma;
    root["gain"] = controls.gain;
    root["power_line_frequency"] = controls.powerLineFrequency;
    root["white_balance_temperature"] = controls.whiteBalanceTemperature;
    root["sharpness"] = controls.sharpness;
    root["backlight_compensation"] = controls.backlightCompensation;
    root["auto_exposure"] = controls.exposureAuto;
    root["exposure_time_absolute"] = controls.exposureTimeAbsolute;
    root["exposure_dynamic_framerate"] = controls.exposure_dynamic_framerate;

    std::ofstream configFile(filename);
    if (configFile.is_open()) {
        configFile << root;
        configFile.close();
        std::cout << "Configuration file created successfully." << std::endl;
        return true;
    } else {
        std::cout << "Failed to create configuration file." << std::endl;
        return false;
    }
}

inline bool CamCtrl::read_cam_config_file(const std::string& filename, CameraControls& controls) {
    std::ifstream configFile(filename);
    if (!configFile.is_open()) {
        std::cout << "Failed to open config file: " << filename << std::endl;
        return false;
    }
    Json::Value configJson;
    configFile >> configJson;
    configFile.close();

    controls.brightness = configJson["brightness"].asInt();
    controls.contrast = configJson["contrast"].asInt();
    controls.saturation = configJson["saturation"].asInt();
    controls.hue = configJson["hue"].asInt();
    controls.autoWhiteBalance = configJson["white_balance_automatic"].asInt();
    controls.gamma = configJson["gamma"].asInt();
    controls.gain = configJson["gain"].asInt();
    controls.powerLineFrequency = configJson["power_line_frequency"].asInt();
    controls.whiteBalanceTemperature = configJson["white_balance_temperature"].asInt();
    controls.sharpness = configJson["sharpness"].asInt();
    controls.backlightCompensation = configJson["backlight_compensation"].asInt();
    controls.exposureAuto = configJson["auto_exposure"].asInt();
    controls.exposureTimeAbsolute = configJson["exposure_time_absolute"].asInt();
    controls.exposure_dynamic_framerate = configJson["exposure_dynamic_framerate"].asInt();

    std::cout << "Config file successfully read: " << filename << std::endl;
    return true;
}

inline bool CamCtrl::load_cam_config(const std::string& filename, CameraControls& controls) {
    if (std::filesystem::exists(filename)) {
        // Config file exists, read the parameters
        std::cout << "Config file exists, reading params from there.." << std::endl;
        return read_cam_config_file(filename, controls);
    } else {
        std::cout << "Config file doesn't exist, creating a new one with default values" << std::endl;
        bool result = false;
        result = create_cam_config_file(filename, controls);
        if(result){
            result = addBoilerplateToJsonFile(filename);
        }
        return result;
    }
}
inline void CamCtrl::set_camera_control(ImageGetter * g, __u32 controlId, __s32 value) {
    struct v4l2_control control;
    control.id = controlId;
    control.value = value;
    cout << "Attmept to write Control "<< std::hex  << controlId << " value: " << std::dec << value << endl;
    if (ioctl(g->fd, VIDIOC_S_CTRL, &control) == -1) { 
        perror("Failed to set camera control");
        cout << "ControlID: " << controlId << endl; 
        // handle error
    } else {
        cout << "Control " << std::hex << controlId << " writen sucessfully!" << std::dec << endl;
    }
}

inline void CamCtrl::set_camera_controls(ImageGetter * g, const CameraControls& controls) {
     cout << "Setting camera controls for fd: " << g->fd << endl;
    // Stop the camera streaming
    if (ioctl(g->fd, VIDIOC_STREAMOFF, &g->bufferinfo.type) == -1) {
        perror("Failed to stop camera streaming");
        return;
    }
    set_camera_control(g, V4L2_CID_BRIGHTNESS, controls.brightness);
    set_camera_control(g, V4L2_CID_CONTRAST, controls.contrast);
    set_camera_control(g, V4L2_CID_SATURATION, controls.saturation);
    set_camera_control(g, V4L2_CID_HUE, controls.hue);
    set_camera_control(g, V4L2_CID_AUTO_WHITE_BALANCE, controls.autoWhiteBalance);
    set_camera_control(g, V4L2_CID_GAMMA, controls.gamma);
    set_camera_control(g, V4L2_CID_GAIN, controls.gain);
    set_camera_control(g, V4L2_CID_POWER_LINE_FREQUENCY, controls.powerLineFrequency);
    set_camera_control(g, V4L2_CID_WHITE_BALANCE_TEMPERATURE, controls.whiteBalanceTemperature);
    set_camera_control(g, V4L2_CID_SHARPNESS, controls.sharpness);
    set_camera_control(g, V4L2_CID_BACKLIGHT_COMPENSATION, controls.backlightCompensation);
    set_camera_control(g, V4L2_CID_EXPOSURE_AUTO, controls.exposureAuto);
    set_camera_control(g, V4L2_CID_EXPOSURE_ABSOLUTE, controls.exposureTimeAbsolute);
    set_camera_control(g, CUSTOM_CID_EXPOSURE_DYNAMIC_FRAMERATE, controls.exposure_dynamic_framerate);
    // Start the camera streaming again
    if (ioctl(g->fd, VIDIOC_STREAMON, &g->bufferinfo.type) == -1) {
        perror("Failed to start camera streaming");
        return;
    }
}
inline bool CamCtrl::is_control_supported(ImageGetter * g, __u32 controlId) {
    struct v4l2_queryctrl queryControl;
    queryControl.id = controlId;
    if (ioctl(g->fd, VIDIOC_QUERYCTRL, &queryControl) == -1) {
        // failed to query control, handle error
        return false;
    }
    if (queryControl.flags & V4L2_CTRL_FLAG_DISABLED) {
        // control is disabled, not supported
        return false;
    }
    return true;
}

inline void CamCtrl::check_camera_capabilities(ImageGetter * g, const char * device) {
        std::vector<__u32> controlIds = {
        V4L2_CID_BRIGHTNESS,
        V4L2_CID_CONTRAST,
        V4L2_CID_SATURATION,
        V4L2_CID_HUE,
        V4L2_CID_AUTO_WHITE_BALANCE,
        V4L2_CID_GAMMA,
        V4L2_CID_GAIN,
        V4L2_CID_POWER_LINE_FREQUENCY,
        V4L2_CID_WHITE_BALANCE_TEMPERATURE,
        V4L2_CID_SHARPNESS,
        V4L2_CID_BACKLIGHT_COMPENSATION,
        V4L2_CID_EXPOSURE_AUTO,
        V4L2_CID_EXPOSURE_ABSOLUTE,
        CUSTOM_CID_EXPOSURE_DYNAMIC_FRAMERATE
    };
    cout << "Checking camera capabilities for fd: " << g->fd << endl;
    for (const auto& controlId : controlIds) {
        if (is_control_supported(g, controlId)) {
            std::cout << "Control " << std::hex << controlId << " is supported" << std::dec << std::endl;
        } else {
            std::cout << "Control " << std::hex << controlId << " is not supported" << std::dec << std::endl;
        }
    }
    struct v4l2_capability capability;
    if (ioctl(g->fd, VIDIOC_QUERYCAP, &capability) < 0) {
        // something went wrong... exit
        perror("Failed to get device capabilities, VIDIOC_QUERYCAP");
        exit(1);
    }		
    printCapabilities(capability);
    close(g->fd);
}


#endif