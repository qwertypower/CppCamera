#pragma once
#include "CommonTypes.h"

class Calibration_impl;
class API_EXPORT Calibration
{
private:
	Calibration_impl* pimpl;
	Calibration& operator=(const Calibration&) = delete;

public:
	Calibration(Calibration_impl* calib);
	virtual ~Calibration();
	Calibration_impl* getImpl();

	Vector2i getSize();
	Vector2f getFocalLength();
	Vector2f getPrincipalPoint();
	Vector4f getDistortionParameters();
	int getRotation();
	Matrix44f getImageProjection(float aspectRatio, int screenRotation);
	Matrix44f getProjectionGL(const float nearPlane, const float farPlane, float aspectRatio, int screenRotation);
};

class Camera_impl;
class API_EXPORT CppCamera
{
private:
	Camera_impl* pimpl;
	CppCamera& operator=(const CppCamera&) = delete;

public:
	CppCamera();
	virtual ~CppCamera();
	Camera_impl* getImpl();

	bool open(int camera);
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
	Calibration* cameraCalibration();
};

