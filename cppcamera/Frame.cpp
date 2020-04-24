#include "Frame.h"
#include "Frame_impl.h"

Frame::Frame()
{
	//pimpl = new Frame_impl();
}
Frame::Frame(Frame_impl* frame_impl)
{
	pimpl = frame_impl;
}
Frame::~Frame()
{
	//delete pimpl;
	//pimpl = nullptr;
}
Frame_impl* Frame::getImpl()
{
	return pimpl;
}
unsigned char* Frame::getFrameData()
{
	return pimpl->img.data;
}
PixelFormat Frame::getFormat()
{
	return pimpl->format;
}
Vector2i Frame::getSize()
{
	return Vector2i{ {{ pimpl->img.cols, pimpl->img.rows }} };
}
double Frame::getTimeStamp()
{
	return 0.0;
}
int Frame::getIndex()
{
	return pimpl->index;
}