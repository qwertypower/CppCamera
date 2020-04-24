#pragma once
#include "CommonTypes.h"

#include <opencv2/core.hpp> 

class Frame_impl
{
public:
	Frame_impl();
	Frame_impl(Frame_impl& other);
	virtual ~Frame_impl();

	cv::Mat img;

	PixelFormat format;
	unsigned int index = 0;
};
