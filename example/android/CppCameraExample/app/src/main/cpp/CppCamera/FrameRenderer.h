#pragma once
#include "CommonTypes.h"
#include "Frame.h"


class FrameRenderer_impl;
class FrameRenderer
{
private:
	std::shared_ptr<FrameRenderer_impl> pimpl;
	FrameRenderer& operator=(const FrameRenderer&) = delete;

public:
	FrameRenderer();
	virtual ~FrameRenderer();
	std::shared_ptr<FrameRenderer_impl> getImpl();

	void upload(PixelFormat format, int width, int height, void* bufferData);
	void upload(Frame* frame);
	void render(Matrix44f imageProjection);
};
