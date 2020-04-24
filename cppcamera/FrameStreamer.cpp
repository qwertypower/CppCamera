#include "FrameStreamer.h"
#include "Frame_impl.h"
#include "FrameStreamer_impl.h"


FrameStreamer::FrameStreamer()
{
	pimpl = new FrameStreamer_impl(3);
}
FrameStreamer::~FrameStreamer()
{
	delete pimpl;
	pimpl = nullptr;
}
FrameStreamer_impl* FrameStreamer::getImpl()
{
	return pimpl;
}
bool FrameStreamer::attachCamera(CppCamera* camera)
{
	return pimpl->attachCamera(camera->getImpl());
}
Frame* FrameStreamer::peek()
{
	auto frame_impl = pimpl->peek();
	if (frame_impl != nullptr && !frame_impl->img.empty())
		return new Frame(frame_impl);
	return nullptr;
}
bool FrameStreamer::start()
{
	return pimpl->start();
}
bool FrameStreamer::stop()
{
	return pimpl->stop();
}