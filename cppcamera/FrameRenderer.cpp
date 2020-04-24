#include "FrameRenderer.h"

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)

#include "FrameRenderer_impl.h"

FrameRenderer::FrameRenderer()
{
	pimpl = std::make_shared<FrameRenderer_impl>();
}
FrameRenderer::~FrameRenderer()
{
	pimpl = nullptr;
}
std::shared_ptr<FrameRenderer_impl> FrameRenderer::getImpl()
{
	return pimpl;
}
void FrameRenderer::upload(PixelFormat format, int width, int height, void* bufferData)
{
	pimpl->upload(format, width, height, bufferData);
}
void FrameRenderer::upload(Frame* frame)
{
	pimpl->upload(frame->getImpl());
}
void FrameRenderer::render(Matrix44f imageProjection)
{
	pimpl->render(imageProjection);
}


#endif