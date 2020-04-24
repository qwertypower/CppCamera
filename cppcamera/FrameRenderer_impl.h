#pragma once
#include "CommonTypes.h"

#if !defined(PLATFORM_PC)

#include "Frame_impl.h"

#if defined PLATFORM_IOS
#   define GLES_SILENCE_DEPRECATION
#   include <OpenGLES/ES2/gl.h>
#   include <cstdio>
#elif defined PLATFORM_ANDROID
#   include <GLES2/gl2.h>
#   include <EGL/egl.h>
#endif

class FrameRenderer_impl
{
public:
	FrameRenderer_impl();
	~FrameRenderer_impl();
	void upload(PixelFormat format, int width, int height, void* bufferData);
	void upload(Frame_impl* frame);
	void render(Matrix44f imageProjection);
private:
	void retrieveFrame(PixelFormat format, int width, int height, void* bufferData, int retrieve_count);
	bool initialize(PixelFormat format);
	void finalize(PixelFormat format);

	bool initialized_;
#ifdef PLATFORM_ANDROID
	EGLContext current_context_;
#endif
	unsigned int background_program_;
	unsigned int background_texture_id_;

	int background_coord_location_;
	int background_texture_location_;
	unsigned int background_coord_vbo_;
	unsigned int background_texture_vbo_;
	unsigned int background_texture_fbo_;

	enum RetrieveStatus
	{
		RetrieveStatus_Unset,
		RetrieveStatus_Upload,
		RetrieveStatus_Clear,
	};

	PixelFormat current_format_;
	Vector2i current_image_size_;
	unsigned int last_frame_index = 0;
};

#endif