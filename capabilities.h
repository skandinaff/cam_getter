#ifndef CAPABILITIES_H
#define CAPABILITIES_H

#include <linux/videodev2.h>
#include <string>
#include <unordered_map>
#include <iostream>

void printCapabilities(const v4l2_capability& cap) {
    std::unordered_map<int, std::string> capabilityMap = {
        {V4L2_CAP_VIDEO_CAPTURE, "Video Capture"},
        {V4L2_CAP_VIDEO_OUTPUT, "Video Output"},
        {V4L2_CAP_VIDEO_OVERLAY, "Video Overlay"},
        {V4L2_CAP_VBI_CAPTURE, "VBI Capture"},
        {V4L2_CAP_VBI_OUTPUT, "VBI Output"},
        {V4L2_CAP_SLICED_VBI_CAPTURE, "Sliced VBI Capture"},
        {V4L2_CAP_SLICED_VBI_OUTPUT, "Sliced VBI Output"},
        {V4L2_CAP_RDS_CAPTURE, "RDS Data Capture"},
        {V4L2_CAP_VIDEO_OUTPUT_OVERLAY, "Video Output Overlay"},
        {V4L2_CAP_HW_FREQ_SEEK, "Hardware Frequency Seek"},
        {V4L2_CAP_RDS_OUTPUT, "RDS Encoder"},
        {V4L2_CAP_VIDEO_CAPTURE_MPLANE, "Multiplanar Video Capture"},
        {V4L2_CAP_VIDEO_OUTPUT_MPLANE, "Multiplanar Video Output"},
        {V4L2_CAP_VIDEO_M2M_MPLANE, "Multiplanar Video Mem-to-Mem"},
        {V4L2_CAP_VIDEO_M2M, "Video Mem-to-Mem"},
        {V4L2_CAP_TUNER, "Tuner"},
        {V4L2_CAP_AUDIO, "Audio"},
        {V4L2_CAP_RADIO, "Radio Device"},
        {V4L2_CAP_MODULATOR, "Modulator"},
        {V4L2_CAP_SDR_CAPTURE, "SDR Capture"},
        {V4L2_CAP_EXT_PIX_FORMAT, "Extended Pixel Format"},
        {V4L2_CAP_SDR_OUTPUT, "SDR Output"},
        {V4L2_CAP_META_CAPTURE, "Metadata Capture"},
        {V4L2_CAP_READWRITE, "Read/Write System Calls"},
        {V4L2_CAP_STREAMING, "Streaming I/O IOCTLs"},
        {V4L2_CAP_META_OUTPUT, "Metadata Output"},
        {V4L2_CAP_TOUCH, "Touch Device"},
        {V4L2_CAP_IO_MC, "Input/Output Controlled by Media Controller"},
        {V4L2_CAP_DEVICE_CAPS, "Device Capabilities"}
    };

    std::cout << "Driver: " << cap.driver << std::endl;
    std::cout << "Card: " << cap.card << std::endl;
    std::cout << "Bus Info: " << cap.bus_info << std::endl;
    std::cout << "Version: " << cap.version << std::endl;
    std::cout << "Capabilities: " << std::hex << cap.capabilities << std::endl;
    std::cout << "Device Caps: " << std::hex << cap.device_caps << std::endl;

    std::cout << "Human-readable capabilities:" << std::endl;
    for (const auto& kv : capabilityMap) {
        if (cap.capabilities & kv.first)
            std::cout << "- " << kv.second << std::endl;
    }
}

#endif