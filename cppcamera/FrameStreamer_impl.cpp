#include "FrameStreamer_impl.h"
#include <opencv2/imgproc.hpp>

FrameStreamer_impl::FrameStreamer_impl(int depth) :
	capacity(depth),
	buffer(new Frame_impl[depth]),
	tail(0),
	head(0)
{
	mframe = new Frame_impl();
	impl = new Frame_impl();
}
FrameStreamer_impl::~FrameStreamer_impl()
{
	delete mframe;
	delete impl;
	mframe = nullptr;
	impl = nullptr;
}
bool FrameStreamer_impl::tryWrite(Frame_impl& frame)
{
	const auto current_tail = tail.load(std::memory_order_relaxed);
	auto next_tail = (current_tail + 1) % capacity;
	if (next_tail != head.load(std::memory_order_acquire))
	{
		//buffer[current_tail].img = frame.img/*.clone()*/;
		frame.img.copyTo(buffer[current_tail].img);
		buffer[current_tail].format = frame.format;
		buffer[current_tail].index = frame.index;
		tail.store(next_tail, std::memory_order_release);
		return true;
	}
	return false;
}
bool FrameStreamer_impl::tryRead(Frame_impl& frame)
{
	const auto current_head = head.load(std::memory_order_relaxed);
	if (current_head == tail.load(std::memory_order_acquire))
	{
		return false;
	}

	frame.img = buffer[current_head].img;
	frame.format = buffer[current_head].format;
	frame.index = buffer[current_head].index;
	auto next_head = (current_head + 1) % capacity;
	head.store(next_head, std::memory_order_release);
	return true;
}
bool FrameStreamer_impl::attachCamera(Camera_impl* camera)
{
	mcamera = camera;
	return mcamera != nullptr;
}
Frame_impl* FrameStreamer_impl::peek()
{
	tryRead(*impl);
	return impl;
}
bool FrameStreamer_impl::start()
{
	if (mcamera == nullptr)
		return false;

#if defined(PLATFORM_ANDROID)
	if (mcamera->android_os_api_level >= 21)
	{
		mcamera->onCameraFrameCallback2 = [this](unsigned char* data1, unsigned char* data2, unsigned char* data3,
			int stride1, int stride2, int stride3,
			int format, int width, int height, int pixel_stride)
		{
			this->frameCallback2(data1, data2, data3, stride1, stride2, stride3, format, width, height, pixel_stride);
		};
	}
	else
	{
		mcamera->onCameraFrameCallback = [this](unsigned char* bytes, int data_size, int width, int height, int format)
		{
			this->frameCallback(bytes, data_size, width, height, format);
		};
	}
#else
	mcamera->onCameraFrameCallback = [this](unsigned char* bytes, int data_size, int width, int height, int format)
	{
		this->frameCallback(bytes, data_size, width, height, format);
	};
#endif
	return true;
}
bool FrameStreamer_impl::stop()
{
	if (mcamera == nullptr)
		return false;
#if defined(PLATFORM_ANDROID)
	if (mcamera->android_os_api_level >= 21)
		mcamera->onCameraFrameCallback2 = nullptr;
	else
		mcamera->onCameraFrameCallback = nullptr;
#else
	mcamera->onCameraFrameCallback = nullptr;
#endif
	return true;
}
void FrameStreamer_impl::frameCallback(unsigned char* bytes, int data_size, int width, int height, int format)
{
	switch (format)
	{
	case PixelFormat::PIX_FMT_RGB8:
		mframe->img = cv::Mat(height, width, CV_8UC3, bytes);
		mframe->format = PixelFormat::PIX_FMT_RGB8;
		break;
	case PixelFormat::PIX_FMT_RGBA8:
		mframe->img = cv::Mat(height, width, CV_8UC4, bytes);
		mframe->format = PixelFormat::PIX_FMT_RGBA8;
		break;
	case PixelFormat::PIX_FMT_BGR8:
		cv::cvtColor(cv::Mat(height, width, CV_8UC3, bytes), mframe->img, cv::COLOR_BGR2BGRA);
		mframe->format = PixelFormat::PIX_FMT_BGRA8;
		break;
	case PixelFormat::PIX_FMT_BGRA8:
		mframe->img = cv::Mat(height, width, CV_8UC4, bytes);
		mframe->format = PixelFormat::PIX_FMT_BGRA8;
		break;
	case PixelFormat::PIX_FMT_NV21:
		cv::cvtColor(cv::Mat(height + height / 2, width, CV_8U, bytes), mframe->img, cv::COLOR_YUV2RGBA_NV21);
		mframe->format = PixelFormat::PIX_FMT_RGBA8;
		break;
	case PixelFormat::PIX_FMT_NV12:
		cv::cvtColor(cv::Mat(height + height / 2, width, CV_8U, bytes), mframe->img, cv::COLOR_YUV2RGBA_NV12);
		mframe->format = PixelFormat::PIX_FMT_RGBA8;
		break;
	};

	mframe->index = frame_index++;
	tryWrite(*mframe);
}
void FrameStreamer_impl::frameCallback2(unsigned char* data1, unsigned char* data2, unsigned char* data3,
	int stride1, int stride2, int stride3,
	int format, int width, int height, int pixel_stride)
{
	switch (format)
	{
	case PixelFormat::PIX_FMT_NV21:
	{
		if (pixel_stride == 2)
		{
			// Chroma channels are interleaved
			cv::Mat y_mat = cv::Mat(height, width, CV_8UC1, data1, stride1);
			cv::Mat uv_mat1 = cv::Mat(height / 2, width / 2, CV_8UC2, data2, stride2);
			cv::Mat uv_mat2 = cv::Mat(height / 2, width / 2, CV_8UC2, data3, stride3);

			long addr_diff = &data3 - &data2;
			if (addr_diff > 0)
			{
				assert(addr_diff == 1);
				cv::cvtColorTwoPlane(y_mat, uv_mat1, mframe->img, cv::COLOR_YUV2RGBA_NV12);
			}
			else
			{
				assert(addr_diff == -1);
				cv::cvtColorTwoPlane(y_mat, uv_mat2, mframe->img, cv::COLOR_YUV2RGBA_NV21);
			}
		}
		else
		{
			cv::Mat yuv_mat = cv::Mat(height + height / 2, width, CV_8UC1);
			unsigned char* yuv_bytes = &yuv_mat.data[0];
			int chromaRowStride = stride2;
			int chromaRowPadding = chromaRowStride - width / 2;
			int offset = width * height;
			memmove(yuv_bytes, data1, offset);
			if (chromaRowPadding == 0)
			{
				// When the row stride of the chroma channels equals their width, we can copy
				// the entire channels in one go
				memmove(&yuv_bytes[offset], data2, width * height / 4);
				offset += width * height / 4;
				memmove(&yuv_bytes[offset], data3, width * height / 4);
			}
			//else
			//{
			//	// When not equal, we need to copy the channels row by row
			//	for (int i = 0; i < height / 2; i++) {
			//		u_plane.get(yuv_bytes, offset, width / 2);
			//		offset += width / 2;
			//		if (i < height / 2 - 1) {
			//			u_plane.position(u_plane.position() + chromaRowPadding);
			//		}
			//	}
			//	for (int i = 0; i < height / 2; i++) {
			//		v_plane.get(yuv_bytes, offset, width / 2);
			//		offset += width / 2;
			//		if (i < height / 2 - 1) {
			//			v_plane.position(v_plane.position() + chromaRowPadding);
			//		}
			//	}
			//}

			cv::cvtColor(yuv_mat, mframe->img, cv::COLOR_YUV2RGBA_I420, 4);
		}

		mframe->format = PixelFormat::PIX_FMT_RGBA8;
		break;
	}
	default: return;
	};

	mframe->index = frame_index++;
	tryWrite(*mframe);
}