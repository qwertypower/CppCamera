#pragma once
#include "CommonTypes.h"
#include "Frame_impl.h"

#if defined(PLATFORM_ANDROID)
#   include "Camera_impl_android.h"
#elif defined(PLATFORM_PC)
#   include "Camera_impl_opencv.h"
#elif defined(PLATFORM_IOS)
#	include "Camera_impl_ios.h"
#endif


#include <atomic>
#include <memory>

class FrameStreamer_impl
{
public:
	FrameStreamer_impl(int depth);
	virtual ~FrameStreamer_impl();
private:
	size_t capacity;
	std::unique_ptr<Frame_impl[]> buffer;
	std::atomic<size_t> tail;
	std::atomic<size_t> head;
	bool tryWrite(Frame_impl& frame);
	bool tryRead(Frame_impl& frame);

public:
	bool attachCamera(Camera_impl* camera);
	Frame_impl* peek();
	bool start();
	bool stop();
	void frameCallback(unsigned char* bytes, int data_size, int width, int height, int format);
	void frameCallback2(unsigned char* data1, unsigned char* data2, unsigned char* data3,
		int stride1, int stride2, int stride3,
		int format, int width, int height, int pixel_stride);
	Camera_impl* mcamera;
	Frame_impl* impl;
	Frame_impl* mframe;
	unsigned int frame_index = 1;
};