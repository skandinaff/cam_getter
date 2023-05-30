// Copyright (c) 2021, Friedrich Doku
/*
SONY IMX335 CMOS, It supports the below resolution & frame rate:
MJPG: 2592x1944@30fps; 2048x1536@20fps; 1920x1080@30fps; 1280x960@30fps; 1280x720@30fps; 1024x768@30fps; 800x600@30fps; 640x480@30fps; 320x240@30fps; 160x120@30fps; 
YUY2: 2592x1944@2fps; 2048x1536@3fps; 1920x1080@3fps; 1280x960@8fps; 1280x720@8fps; 960x540@15fps; 800x600@20fps; 640x480@30fps; 
*/
#ifndef CAM_H
#define CAM_H


#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/v4l2-common.h>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fstream>
#include <iostream>
#include <string>
#include <string.h>
#include <cstring>
#include <map>
#include <vector>

using namespace std;

struct Resolution {
    int width;
    int height;
};

std::map<std::string, Resolution> resolutions = {
    {"VGA", {640, 480}},
    {"XGA", {1024, 768}},
    {"WXGAPlus", {2592, 1944}},
    {"FullHD", {1920, 1080}}
};

typedef struct {
	int					fd;				// File descriptor
	struct v4l2_format			imageFormat;	// Image Format
	struct v4l2_requestbuffers requestBuffer;
	struct v4l2_buffer			queryBuffer;
	struct v4l2_buffer			bufferinfo;
	char *				buffer;
} ImageGetter;

void initialize_imget(ImageGetter * g, const char * device)
{
	//TODO: Make it so a device can be chosen
	g->fd = open(device, O_RDWR);
	if (g->fd < 0) {
		perror("Failed to open device, OPEN");
		//exit(1);
	} else cout << "Camera Opened!" << endl;


	// Ask the device if it can capture frames
	{
		struct v4l2_capability capability;
		if (ioctl(g->fd, VIDIOC_QUERYCAP, &capability) < 0) {
			// something went wrong... exit
			perror("Failed to get device capabilities, VIDIOC_QUERYCAP");
			exit(1);
		}
	}
}

bool is_control_supported(ImageGetter * g, __u32 controlId) {
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

    // control is supported
    return true;
}

void check_camera_capabilities(ImageGetter * g, const char * device) {
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

void set_img_format(ImageGetter * g, const Resolution& res)
{
	// Set Image format
	g->imageFormat.type				   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	g->imageFormat.fmt.pix.width	   = res.width;
	g->imageFormat.fmt.pix.height	   = res.height;
	g->imageFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;//V4L2_PIX_FMT_MJPEG;
	g->imageFormat.fmt.pix.field	   = V4L2_FIELD_NONE;
	// tell the device you are using this format
	int ret = 0;
	if (ret = ioctl(g->fd, VIDIOC_S_FMT, &g->imageFormat) < 0) {
		perror("Device could not set format, VIDIOC_S_FMT");
		//exit(1);
	}
	cout << "Set img format resulted with: " << ret << endl; 
}

void set_camera_control(ImageGetter * g, __u32 controlId, __s32 value) {
    struct v4l2_control control;
    control.id = controlId;
    control.value = value;
    
    if (ioctl(g->fd, VIDIOC_S_CTRL, &control) == -1) {
        perror("Failed to set camera control");
        // handle error
    }
}


void 
setup_buffers(ImageGetter * g)
{
	// Request Buffers from the device
	//g->requestBuffer		= {0};
	g->requestBuffer.count	= 1;							  // one request buffer
	g->requestBuffer.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;	  // request a buffer to use for capturing frames
	g->requestBuffer.memory = V4L2_MEMORY_MMAP;

	if (ioctl(g->fd, VIDIOC_REQBUFS, &g->requestBuffer) < 0) {
		perror("Could not request buffer from device, VIDIOC_REQBUFS");
		//exit(1)
	}

	// Query the buffer to get raw data
	//g->queryBuffer		  = {0};
	g->queryBuffer.type	  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	g->queryBuffer.memory = V4L2_MEMORY_MMAP;
	g->queryBuffer.index  = 0;
	if (ioctl(g->fd, VIDIOC_QUERYBUF, &g->queryBuffer) < 0) {
		perror("Device did not return the buffer information, VIDIOC_QUERYBUF");
		//exit(1);
	}
}

int pre_grab_frame(ImageGetter * g) {
    // mmap() will map the memory address of the device to an address in memory
    cout << "mapping memory addr" << endl;
    g->buffer = (char *)mmap(NULL, g->queryBuffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, g->fd, g->queryBuffer.m.offset);
    cout << "setting buffer to 0" << endl;
    memset(g->buffer, 0, g->queryBuffer.length);

    // Create a new buffer type so the device knows which buffer we are talking about
    cout << "create a new buffer" << endl;
    g->bufferinfo;
    memset(&g->bufferinfo, 0, sizeof(g->bufferinfo));
    g->bufferinfo.type     = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    g->bufferinfo.memory   = V4L2_MEMORY_MMAP;
    g->bufferinfo.index    = 0;

    // Activate streaming
    cout << "activate streaming" << endl;
    int type = g->bufferinfo.type;
    if (ioctl(g->fd, VIDIOC_STREAMON, &type) < 0) {
        perror("Could not start streaming, VIDIOC_STREAMON");
        return -1;
    }

    // Queue the buffer
	/*
    if (ioctl(g->fd, VIDIOC_QBUF, &g->bufferinfo) < 0) {
        perror("Could not queue buffer, VIDIOC_QBUF");
        return -1;
    }
	*/
    return 0;
}


int grab_frame(ImageGetter * g)
{
	// mmap() will map the memory address of the device to an address in memory
	cout << "mapping memory addr" << endl;
	g->buffer = (char *)mmap(NULL, g->queryBuffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, g->fd, g->queryBuffer.m.offset);
	cout << "setting buffer to 0" << endl;
	memset(g->buffer, 0, g->queryBuffer.length);

	// Create a new buffer type so the device knows which buffer we are talking about
	cout << "create a new buffer" << endl;
	g->bufferinfo;
	memset(&g->bufferinfo, 0, sizeof(g->bufferinfo));
	g->bufferinfo.type	 = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	g->bufferinfo.memory = V4L2_MEMORY_MMAP;
	g->bufferinfo.index	 = 0;

	// Activate streaming
	cout << "activate streaming" << endl;
	int type = g->bufferinfo.type;
	if (ioctl(g->fd, VIDIOC_STREAMON, &type) < 0) {
		perror("Could not start streaming, VIDIOC_STREAMON");
		return -1;
	}


	// Queue the buffer
	if (ioctl(g->fd, VIDIOC_QBUF, &g->bufferinfo) < 0) {
		perror("Could not queue buffer, VIDIOC_QBUF");
		return -1;
	}

	// Dequeue the buffer
	if (ioctl(g->fd, VIDIOC_DQBUF, &g->bufferinfo) < 0) {
		perror("Could not dequeue the buffer, VIDIOC_DQBUF");
		return -1;
	}

	// end streaming
	if (ioctl(g->fd, VIDIOC_STREAMOFF, &type) < 0) {
		perror("Could not end streaming, VIDIOC_STREAMOFF");
		return 1;
	}

	printf("Buffer has: %f",(double)g->bufferinfo.bytesused / 1024);
    printf(" KBytes of data\n");

	//close(g->fd);

	return 0;
}

int grab_frame2(ImageGetter * g) {
	// Queue the buffer
    
	if (ioctl(g->fd, VIDIOC_QBUF, &g->bufferinfo) < 0) {
        perror("Could not queue buffer, VIDIOC_QBUF");
        return -1;
    }
    // Dequeue the buffer
	cout << "Deququeing the buffer.." << endl;
    if (ioctl(g->fd, VIDIOC_DQBUF, &g->bufferinfo) < 0) {
        perror("Could not dequeue the buffer, VIDIOC_DQBUF");
        return -1;
    }

    printf("Buffer has: %f",(double)g->bufferinfo.bytesused / 1024);
    printf(" KBytes of data\n");

    return 0;
}

int post_grab_frame(ImageGetter * g) {
    // end streaming
    int type = g->bufferinfo.type;
    if (ioctl(g->fd, VIDIOC_STREAMOFF, &type) < 0) {
        perror("Could not end streaming, VIDIOC_STREAMOFF");
        return -1;
    }

    close(g->fd);

    return 0;
}

int save_buffer(char *buffer, size_t buffer_size, const std::string& filename) {
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return -1;
    }

    if (!ofs.write(buffer, buffer_size)) {
        std::cerr << "Could not write buffer to file: " << filename << std::endl;
        return -1;
    }

    std::cout << "Buffer saved to " << filename << std::endl;

    return 0;
}
int save_buffer_as_array(char *buffer, size_t buffer_size, const std::string& filename, int width, int height, int bytes_per_pixel) {
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return -1;
    }

    // Calculate the number of bytes per row
    int bytes_per_row = width * bytes_per_pixel;

    // Create a 2D byte array of the appropriate size
    std::vector<std::vector<unsigned char>> image_data(height, std::vector<unsigned char>(width));

    // Copy the image data into the 2D array
	cout << "Copying bytes..." << endl;
    int buffer_index = 0;
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < bytes_per_row; col++) {
            image_data[row][col] = buffer[buffer_index++];
        }
    }
    cout << "Writing bytes into the file..." << endl;
    // Write the 2D byte array to file
	
    for (int row = 0; row < height; row++) {
        ofs.write(reinterpret_cast<char*>(image_data[row].data()), bytes_per_row);
    }

    std::cout << "Buffer saved to " << filename << std::endl;

    return 0;
}

int PrepareCamera(ImageGetter* g, std::string dev){
    initialize_imget(g, dev.c_str());
    set_img_format(g, resolutions["WXGAPlus"]);
	setup_buffers(g);
	pre_grab_frame(g);
	return 0;
}

#endif

