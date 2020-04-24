#pragma once
#include "CommonTypes.h"
#include "CppCamera.h"
#include "Frame.h"


class FrameStreamer_impl;
class API_EXPORT FrameStreamer
{
private:
	FrameStreamer_impl* pimpl;
public:
	FrameStreamer();
	virtual ~FrameStreamer();
	FrameStreamer_impl* getImpl();

	bool attachCamera(CppCamera* camera);
	Frame* peek();
	bool start();
	bool stop();
};
