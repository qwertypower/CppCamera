#include "bridge.h"

#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include <string>

#include "CppCamera/CppCamera.h"
#include "CppCamera/FrameStreamer.h"
#include "CppCamera/FrameRenderer.h"

#include <opencv2/opencv.hpp>

CppCamera* camera;
FrameStreamer* streamer;
FrameRenderer* renderer;
Vector2i preview_size = Vector2i{ {{640, 480}} };
bool flash_enabled = false;

DeviceType devtype = DeviceType::BACK;

void switchCamera()
{
	if (devtype == DeviceType::BACK)
		devtype = DeviceType::FRONT;
	else
		devtype = DeviceType::BACK;

	streamer->stop();
	camera->stop();

	delete streamer;
	streamer = new FrameStreamer();
	delete camera;
	camera = new CppCamera();

	camera->open(devtype);
	streamer->attachCamera(camera);
	camera->setSize(preview_size);
	camera->start();
	camera->setFocusMode(FocusMode::ContiniousAuto);

	streamer->start();
}

void enableFlashlight()
{
	if (!flash_enabled)
		camera->setFlashTorchMode(true);
	else
		camera->setFlashTorchMode(false);
	flash_enabled = !flash_enabled;
}

bool initialize()
{
	//freopen("/sdcard/runlog.txt", "w", stdout);
	if (camera == nullptr)
		camera = new CppCamera();
	bool res = camera->open(devtype);
	if (streamer == nullptr)
		streamer = new FrameStreamer();
	streamer->attachCamera(camera);
	camera->setSize(preview_size);
	return res;
}

bool start()
{
	bool status = true;
	status &= camera->start();
	status &= camera->setFocusMode(FocusMode::ContiniousAuto);
	status &= (streamer != nullptr) && streamer->start();
	return status;
}

bool stop()
{
	bool status = true;
	status &= (streamer != nullptr) && streamer->stop();
	status &= (camera != nullptr) && camera->stop();
	return status;
}

void finalize()
{
	delete renderer;
	delete streamer;
	delete camera;
	renderer = nullptr;
	streamer = nullptr;
	camera = nullptr;
}

void initGL()
{
	renderer = new FrameRenderer();
}

void render(int width, int height, int rotation)
{
	glViewport(0, 0, width, height);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	auto frame = streamer->peek();
	if (frame == nullptr) return;

    cv::Mat rgba = cv::Mat(frame->getSize().data[1], frame->getSize().data[0], CV_8UC4, frame->getFrameData());
    cv::putText(rgba, "YOBA", cv::Point(320, 240), cv::FONT_HERSHEY_PLAIN, 2, cv::Scalar(0, 255, 0), 2);


	auto calibration = camera->cameraCalibration();
	float viewport_aspect_ratio = (float)width / (float)height;
	Matrix44f imageProjection = calibration->getImageProjection(viewport_aspect_ratio, rotation);
	
	if (renderer != nullptr) 
	{
		renderer->upload(frame);
		renderer->render(imageProjection);
	}

	delete calibration;
	delete frame;
}

void onResume()
{
	initialize();
	start();
}

void onPause()
{
	stop();
}

void onDestroy()
{
	stop();
	finalize();
}
