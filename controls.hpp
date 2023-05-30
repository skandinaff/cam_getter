#ifndef CONTROLS_HPP
#define CONTROLS_HPP

#include <fstream>
#include <iostream>
#include <filesystem>
#include <jsoncpp/json/json.h>

#include "cam.h"

// v4l2-ctl -d /dev/video1 -l // command to check cameras controls

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
    };
    bool create_cam_config_file(const std::string& filename, const CameraControls& controls);
    bool read_cam_config_file(const std::string& filename, CameraControls& controls);
    bool load_cam_config(const std::string& filename, CameraControls& controls);
    void set_camera_control(ImageGetter * g, __u32 controlId, __s32 value);
    void set_camera_controls(ImageGetter * g, const CameraControls& controls);
    void check_camera_capabilities(ImageGetter * g, const char * device);
    bool is_control_supported(ImageGetter * g, __u32 controlId);




};

inline bool CamCtrl::create_cam_config_file(const std::string& filename, const CameraControls& controls) {
    Json::Value root;
    root["brightness"] = controls.brightness;
    root["contrast"] = controls.contrast;
    root["saturation"] = controls.saturation;
    root["hue"] = controls.hue;
    root["autoWhiteBalance"] = controls.autoWhiteBalance;
    root["gamma"] = controls.gamma;
    root["gain"] = controls.gain;
    root["powerLineFrequency"] = controls.powerLineFrequency;
    root["whiteBalanceTemperature"] = controls.whiteBalanceTemperature;
    root["sharpness"] = controls.sharpness;
    root["backlightCompensation"] = controls.backlightCompensation;
    root["exposureAuto"] = controls.exposureAuto;
    root["exposureTimeAbsolute"] = controls.exposureTimeAbsolute;

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
    controls.autoWhiteBalance = configJson["autoWhiteBalance"].asInt();
    controls.gamma = configJson["gamma"].asInt();
    controls.gain = configJson["gain"].asInt();
    controls.powerLineFrequency = configJson["powerLineFrequency"].asInt();
    controls.whiteBalanceTemperature = configJson["whiteBalanceTemperature"].asInt();
    controls.sharpness = configJson["sharpness"].asInt();
    controls.backlightCompensation = configJson["backlightCompensation"].asInt();
    controls.exposureAuto = configJson["exposureAuto"].asInt();
    controls.exposureTimeAbsolute = configJson["exposureTimeAbsolute"].asInt();

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
        return create_cam_config_file(filename, controls);
    }
}
inline void CamCtrl::set_camera_control(ImageGetter * g, __u32 controlId, __s32 value) {
    struct v4l2_control control;
    control.id = controlId;
    control.value = value;
    
    if (ioctl(g->fd, VIDIOC_G_CTRL, &control) == -1) {
        perror("Failed to set camera control");
        cout << "ControlID: " << controlId << endl; 
        // handle error
    } else {
        cout << "Control " << controlId << "writen sucessfully!" << endl;
    }
}

inline void CamCtrl::set_camera_controls(ImageGetter * g, const CameraControls& controls) {
    set_camera_control(g, 0x00980900, controls.brightness); // V4L2_CID_BRIGHTNESS
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

    cout << "Control supported!" << endl;
    return true;
}

inline void CamCtrl::check_camera_capabilities(ImageGetter * g, const char * device) {
    g->fd = open(device, O_RDWR);
    if (g->fd == -1) {
        perror("Failed to open device");
        return;
    }

    std::vector<__u32> controlIds = {
        V4L2_CID_BRIGHTNESS,
        V4L2_CID_CONTRAST,
        V4L2_CID_SATURATION,
        V4L2_CID_HUE,
        V4L2_CID_AUTO_WHITE_BALANCE,
		V4L2_CID_DO_WHITE_BALANCE,
        V4L2_CID_GAMMA,
        V4L2_CID_GAIN,
        V4L2_CID_POWER_LINE_FREQUENCY,
        V4L2_CID_WHITE_BALANCE_TEMPERATURE,
        V4L2_CID_SHARPNESS,
        V4L2_CID_BACKLIGHT_COMPENSATION,
        V4L2_CID_EXPOSURE_AUTO
    };

    for (const auto& controlId : controlIds) {
        if (is_control_supported(g, controlId)) {
            std::cout << "Control " << std::hex << controlId << " is supported" << std::endl;
        } else {
            std::cout << "Control " << std::hex << controlId << " is not supported" << std::endl;
        }
    }

    close(g->fd);
}


#endif