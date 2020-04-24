#include "FrameRenderer_impl.h"

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)

const char* videobackground_vert =
"attribute vec4 coord;\n"
"attribute vec2 texCoord;\n"
"varying vec2 texc;\n"
"\n"
"void main(void)\n"
"{\n"
"    gl_Position = coord;\n"
"    texc = texCoord;\n"
"}\n"
;
const char* videobackground_bgr_frag =
"#ifdef GL_ES\n"
"precision mediump float;\n"
"#endif\n"
"uniform sampler2D texture;\n"
"varying vec2 texc;\n"
"\n"
"void main(void)\n"
"{\n"
"    gl_FragColor = texture2D(texture, texc).bgra;\n"
"}\n"
;
const char* videobackground_rgb_frag =
"#ifdef GL_ES\n"
"precision mediump float;\n"
"#endif\n"
"uniform sampler2D texture;\n"
"varying vec2 texc;\n"
"\n"
"void main(void)\n"
"{\n"
"    gl_FragColor = texture2D(texture, texc);\n"
"}\n"
;


Vector4f mul(const Matrix44f& lhs, const Vector4f& rhs)
{
	Vector4f v = { {{0.0f, 0.0f, 0.0f, 0.0f}} };
	for (int i = 0; i < 4; i += 1)
	{
		for (int k = 0; k < 4; k += 1)
		{
			v.data[i] += lhs.data[i * 4 + k] * rhs.data[k];
		}
	}
	return v;
}

#include <GLES3/gl31.h>

FrameRenderer_impl::FrameRenderer_impl()
{
#ifdef PLATFORM_ANDROID
	current_context_ = eglGetCurrentContext();
#endif
	initialized_ = false;
	background_program_ = 0;
	background_texture_id_ = 0;
	background_coord_location_ = -1;
	background_texture_location_ = -1;
	background_coord_vbo_ = 0;
	background_texture_vbo_ = 0;
	background_texture_fbo_ = 0;
	current_image_size_ = { {{0, 0}} };
}

FrameRenderer_impl::~FrameRenderer_impl()
{
	finalize(current_format_);
}

void FrameRenderer_impl::upload(PixelFormat format, int width, int height, void* bufferData)
{
	GLint bak_tex, bak_program, bak_active_tex, bak_tex_1, bak_tex_2;
	glGetIntegerv(GL_CURRENT_PROGRAM, &bak_program);
	glGetIntegerv(GL_ACTIVE_TEXTURE, &bak_active_tex);
	glActiveTexture(GL_TEXTURE0);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &bak_tex);
	glActiveTexture(GL_TEXTURE1);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &bak_tex_1);
	glActiveTexture(GL_TEXTURE2);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &bak_tex_2);

	do
	{
		if (current_format_ != format)
		{
			finalize(current_format_);
			if (!initialize(format)) { break; }
			current_format_ = format;
		}
		current_image_size_ = Vector2i({ {{ width, height }} });
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, background_texture_id_);
		retrieveFrame(format, width, height, bufferData, 0);

	} while (0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bak_tex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bak_tex_1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, bak_tex_2);
	glActiveTexture(bak_active_tex);
	glUseProgram(bak_program);
}

void FrameRenderer_impl::upload(Frame_impl* frame)
{
	if (frame->index != last_frame_index)
		upload(frame->format, frame->img.cols, frame->img.rows, frame->img.data);

	last_frame_index = frame->index;
}

void FrameRenderer_impl::render(Matrix44f imageProjection)
{
	GLint bak_blend, bak_depth, bak_fbo, bak_tex, bak_arr_buf, bak_ele_arr_buf, bak_cull, bak_program, bak_active_tex, bak_tex_1, bak_tex_2;
	GLint bak_viewport[4];
	glGetIntegerv(GL_BLEND, &bak_blend);
	glGetIntegerv(GL_DEPTH_TEST, &bak_depth);
	glGetIntegerv(GL_CULL_FACE, &bak_cull);
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bak_arr_buf);
	glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &bak_ele_arr_buf);
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bak_fbo);
	glGetIntegerv(GL_VIEWPORT, &bak_viewport[0]);
	glGetIntegerv(GL_CURRENT_PROGRAM, &bak_program);
	glGetIntegerv(GL_ACTIVE_TEXTURE, &bak_active_tex);
	glActiveTexture(GL_TEXTURE0);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &bak_tex);
	glActiveTexture(GL_TEXTURE1);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &bak_tex_1);
	glActiveTexture(GL_TEXTURE2);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &bak_tex_2);

	int va[2] = { -1, -1 };
	int bak_va_binding[2] = { 0, 0 };
	int bak_va_enable[2], bak_va_size[2], bak_va_stride[2], bak_va_type[2], bak_va_norm[2];
	void* bak_va_pointer[2];

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	va[0] = background_coord_location_;
	va[1] = background_texture_location_;
	for (int i = 0; i < 2; ++i) {
		if (va[i] == -1)
			continue;
		glGetVertexAttribiv(va[i], GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &bak_va_binding[i]);
		glGetVertexAttribiv(va[i], GL_VERTEX_ATTRIB_ARRAY_ENABLED, &bak_va_enable[i]);
		glGetVertexAttribiv(va[i], GL_VERTEX_ATTRIB_ARRAY_SIZE, &bak_va_size[i]);
		glGetVertexAttribiv(va[i], GL_VERTEX_ATTRIB_ARRAY_STRIDE, &bak_va_stride[i]);
		glGetVertexAttribiv(va[i], GL_VERTEX_ATTRIB_ARRAY_TYPE, &bak_va_type[i]);
		glGetVertexAttribiv(va[i], GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &bak_va_norm[i]);
		glGetVertexAttribPointerv(va[i], GL_VERTEX_ATTRIB_ARRAY_POINTER, &bak_va_pointer[i]);
	}

	bool result = false;
	do {
		glUseProgram(background_program_);
		glBindBuffer(GL_ARRAY_BUFFER, background_coord_vbo_);

		glEnableVertexAttribArray(background_coord_location_);
		glVertexAttribPointer(background_coord_location_, 3, GL_FLOAT, GL_FALSE, 0, 0);

		GLfloat vertices[] = {
			-1.0f,  -1.0f,  0.f,
			 1.0f,  -1.0f,  0.f,
			 1.0f,   1.0f,  0.f,
			-1.0f,   1.0f,  0.f,
		};

		Vector4f v0 = mul(imageProjection, Vector4f({ {{vertices[0], vertices[1], vertices[2], 1}} }));
		Vector4f v1 = mul(imageProjection, Vector4f({ {{vertices[3], vertices[4], vertices[5], 1}} }));
		Vector4f v2 = mul(imageProjection, Vector4f({ {{vertices[6], vertices[7], vertices[8], 1}} }));
		Vector4f v3 = mul(imageProjection, Vector4f({ {{vertices[9], vertices[10], vertices[11], 1}} }));
		memcpy(&vertices[0], &v0.data[0], 3 * sizeof(float));
		memcpy(&vertices[3], &v1.data[0], 3 * sizeof(float));
		memcpy(&vertices[6], &v2.data[0], 3 * sizeof(float));
		memcpy(&vertices[9], &v3.data[0], 3 * sizeof(float));

		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 12, vertices, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, background_texture_vbo_);
		glEnableVertexAttribArray(background_texture_location_);
		glVertexAttribPointer(background_texture_location_, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, background_texture_id_);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		result = true;
	} while (0);

	if (bak_blend) glEnable(GL_BLEND);
	if (bak_depth) glEnable(GL_DEPTH_TEST);
	if (bak_cull) glEnable(GL_CULL_FACE);

	for (int i = 0; i < 2; ++i) {
		if (!bak_va_binding[i])
			continue;
		glBindBuffer(GL_ARRAY_BUFFER, bak_va_binding[i]);
		if (bak_va_enable[i])
			glEnableVertexAttribArray(va[i]);
		else
			glDisableVertexAttribArray(va[i]);
		glVertexAttribPointer(va[i], bak_va_size[i], bak_va_type[i], (GLboolean)bak_va_norm[i], bak_va_stride[i], bak_va_pointer[i]);
	}

	glBindBuffer(GL_ARRAY_BUFFER, bak_arr_buf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bak_ele_arr_buf);
	glBindFramebuffer(GL_FRAMEBUFFER, bak_fbo);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bak_tex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bak_tex_1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, bak_tex_2);
	glActiveTexture(bak_active_tex);
	glViewport(bak_viewport[0], bak_viewport[1], bak_viewport[2], bak_viewport[3]);
	glUseProgram(bak_program);
}

void FrameRenderer_impl::retrieveFrame(PixelFormat format, int width, int height, void* bufferData, int retrieve_count)
{

	if (width & 0x3) {
		if (width & 0x1)
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		else
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
	}

	switch (format) {
	case PixelFormat::PIX_FMT_UNK:
		glBindTexture(GL_TEXTURE_2D, 0);
		break;
	case PixelFormat::PIX_FMT_GRAY:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, bufferData);
		break;
	case PixelFormat::PIX_FMT_BGR8:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, bufferData);
		break;
	case PixelFormat::PIX_FMT_RGB8:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, bufferData);
		break;
	case PixelFormat::PIX_FMT_RGBA8:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bufferData);
		break;
	case PixelFormat::PIX_FMT_BGRA8:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bufferData);
		break;
	default:
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

bool FrameRenderer_impl::initialize(PixelFormat format)
{
	if (format == PixelFormat::PIX_FMT_UNK)
		return false;
	background_program_ = glCreateProgram();
	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertShader, 1, &videobackground_vert, NULL);
	glCompileShader(vertShader);
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);

	switch (format) {
	case PixelFormat::PIX_FMT_GRAY:
	case PixelFormat::PIX_FMT_RGB8:
	case PixelFormat::PIX_FMT_RGBA8:
		glShaderSource(fragShader, 1, &videobackground_rgb_frag, NULL);
		break;
	case PixelFormat::PIX_FMT_BGR8:
	case PixelFormat::PIX_FMT_BGRA8:
		glShaderSource(fragShader, 1, &videobackground_bgr_frag, NULL);
		break;
	default:
		break;
	}
	glCompileShader(fragShader);
	glAttachShader(background_program_, vertShader);
	glAttachShader(background_program_, fragShader);

	GLint compileSuccess;
	glGetShaderiv(vertShader, GL_COMPILE_STATUS, &compileSuccess);
	if (compileSuccess == GL_FALSE) {
		GLchar messages[256];
		glGetShaderInfoLog(vertShader, sizeof(messages), 0, &messages[0]);
	}
	glGetShaderiv(fragShader, GL_COMPILE_STATUS, &compileSuccess);
	if (compileSuccess == GL_FALSE) {
		GLchar messages[256];
		glGetShaderInfoLog(fragShader, sizeof(messages), 0, &messages[0]);
	}
	glLinkProgram(background_program_);
	glDeleteShader(vertShader);
	glDeleteShader(fragShader);
	GLint linkstatus = 0;

	glGetProgramiv(background_program_, GL_LINK_STATUS, &linkstatus);
	glUseProgram(background_program_);
	background_coord_location_ = glGetAttribLocation(background_program_, "coord");
	background_texture_location_ = glGetAttribLocation(background_program_, "texCoord");

	glGenBuffers(1, &background_coord_vbo_);
	glBindBuffer(GL_ARRAY_BUFFER, background_coord_vbo_);
	const GLfloat coord[] = { -1.0f, -1.0f, 0.f, 1.0f, -1.0f, 0.f, 1.0f, 1.0f, 0.f, -1.0f, 1.0f, 0.f };
	glBufferData(GL_ARRAY_BUFFER, sizeof(coord), coord, GL_DYNAMIC_DRAW);
	glGenBuffers(1, &background_texture_vbo_);
	glBindBuffer(GL_ARRAY_BUFFER, background_texture_vbo_);
	static const GLfloat texcoord[] = { 0.f, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f }; //input texture data is Y-inverted
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), texcoord, GL_STATIC_DRAW);

	glUniform1i(glGetUniformLocation(background_program_, "texture"), 0);
	glGenTextures(1, &background_texture_id_);
	glBindTexture(GL_TEXTURE_2D, background_texture_id_);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenFramebuffers(1, &background_texture_fbo_);
	initialized_ = true;
	return true;
}

void FrameRenderer_impl::finalize(PixelFormat format)
{
	if (!initialized_) { return; }
#ifdef PLATFORM_ANDROID
	if (eglGetCurrentContext() == current_context_)
#endif
	{
		glGetError();

		glDeleteProgram(background_program_);
		glDeleteBuffers(1, &background_coord_vbo_);
		glDeleteBuffers(1, &background_texture_vbo_);
		glDeleteFramebuffers(1, &background_texture_fbo_);
		glDeleteTextures(1, &background_texture_id_);

		initialized_ = false;

		glGetError();
	}
}

#endif