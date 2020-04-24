#include "Frame_impl.h"

Frame_impl::Frame_impl()
{
	this->format = PixelFormat::PIX_FMT_UNK;
}

Frame_impl::Frame_impl(Frame_impl& other)
{
	//other.img.copyTo(this->img);
	this->img = other.img;
	this->index = other.index;
	this->format = other.format;
}

Frame_impl::~Frame_impl()
{
}
