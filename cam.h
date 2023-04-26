// Copyright (c) 2021, Friedrich Doku

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

using namespace std;

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

void set_img_format(ImageGetter * g)
{
	// Set Image format
	g->imageFormat.type				   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	g->imageFormat.fmt.pix.width	   = 1024;
	g->imageFormat.fmt.pix.height	   = 1024;
	g->imageFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	g->imageFormat.fmt.pix.field	   = V4L2_FIELD_NONE;
	// tell the device you are using this format
	if (ioctl(g->fd, VIDIOC_S_FMT, &g->imageFormat) < 0) {
		perror("Device could not set format, VIDIOC_S_FMT");
		//exit(1);
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

void PrepareCamera(ImageGetter* g, std::string dev){
    initialize_imget(g, dev.c_str());
    set_img_format(g);
	setup_buffers(g);
}

#endif

