#pragma once
#include "CommonTypes.h"


class Frame_impl;
class API_EXPORT Frame
{
private:
	Frame_impl* pimpl;
	Frame& operator=(const Frame&) = delete;

public:
	Frame();
	Frame(Frame_impl* frame_impl);
	virtual ~Frame();
	Frame_impl* getImpl();

	unsigned char* getFrameData();
	PixelFormat getFormat();

	Vector2i getSize();
	double getTimeStamp();
	int getIndex();
};
