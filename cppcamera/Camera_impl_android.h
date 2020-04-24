#pragma once

#include "CommonTypes.h"

#ifdef PLATFORM_ANDROID

#include <jni.h>
#include <opencv2/core.hpp>


class Calibration_impl
{
public:
	Calibration_impl();
	Vector2i getSize();
	Vector2f getFocalLength();
	Vector2f getPrincipalPoint();
	Vector4f getDistortionParameters();
	int getRotation();
	Matrix44f getImageProjection(float aspectRatio, int screenRotation);
	Matrix44f getProjectionGL(const float nearPlane, const float farPlane, float aspectRatio, int screenRotation);
	void calibrate(float fov, float fwidth, float fheight);

	cv::Mat K;
	int rotation = 0;
	bool frontal = false;
	float width{ 640.f };
	float height{ 480.f };
	float diagFOV = -1.f;
	float focal = 3.5;
};


class Camera_impl
{
public:
	Camera_impl();
	virtual ~Camera_impl();

	bool open(int type);
	bool start();
	bool stop();
	bool close();

	Vector2i size();
	int supportedSizeCount();
	Vector2i supportedSize(int index);
	bool setSize(Vector2i size);
	int supportedFrameRateRangeCount();
	float supportedFrameRateRangeLower(int index);
	float supportedFrameRateRangeUpper(int index);
	int frameRateRange();
	bool setFrameRateRange(int index);
	bool setFlashTorchMode(bool on);
	bool setFocusMode(FocusMode focusMode);
	bool autoFocus();

	Calibration_impl* cameraCalibration();
	Calibration_impl* camera_calibration;
	std::function<void(unsigned char* bytes, int data_size, int width, int height, int pixel_format)> onCameraFrameCallback = nullptr;
	std::function<void(unsigned char* data1, unsigned char* data2, unsigned char* data3, 
		int stride1, int stride2, int stride3, 
		int pixel_format, int width, int height, int pixel_stride)> onCameraFrameCallback2 = nullptr;
	Vector2i preview_size{ {{1280, 720}} };

	// bullshits
	int android_os_api_level;

private:

	/* JNI METHODS */

	jobject camobj;
	jclass cam;

	jmethodID jsetCameraDeviceRawPointer;
	jmethodID jopen;
	jmethodID jstart;
	jmethodID jstopAndClose;
	jmethodID jisCamera2;
	jmethodID jisFrontFacing;
	jmethodID jgetCameraOrientation;
	jmethodID jgetSizeWidth;
	jmethodID jgetSizeHeight;
	jmethodID jgetNumSupportedSize;
	jmethodID jgetSupportedSizeWidth;
	jmethodID jgetSupportedSizeHeight;
	jmethodID jsetSize;
	jmethodID jgetNumSupportedFrameRateRange;
	jmethodID jgetSupportedFrameRateRangeLower;
	jmethodID jgetSupportedFrameRateRangeUpper;
	jmethodID jgetFrameRateRange;
	jmethodID jsetFrameRateRange;
	jmethodID jsetFlashTorchMode;
	jmethodID jsetFocusMode;
	jmethodID jautoFocus;
};


#endif