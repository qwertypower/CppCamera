#include "CppCamera.h"

#if defined(PLATFORM_ANDROID)
#   include "Camera_impl_android.h"
#elif defined(PLATFORM_PC)
#   include "Camera_impl_opencv.h"
#elif defined(PLATFORM_IOS)
#	include "Camera_impl_ios.h"
#endif


Calibration::Calibration(Calibration_impl* calib)
{
	pimpl = calib;
}
Calibration::~Calibration()
{
}
Calibration_impl* Calibration::getImpl()
{
	return pimpl;
}
Vector2i Calibration::getSize()
{
	return pimpl->getSize();
}
Vector2f Calibration::getFocalLength()
{
	return pimpl->getFocalLength();
}
Vector2f Calibration::getPrincipalPoint()
{
	return pimpl->getPrincipalPoint();
}
Vector4f Calibration::getDistortionParameters()
{
	return { {{0, 0, 0, 0}} };
}
int Calibration::getRotation()
{
	return pimpl->getRotation();
}
Matrix44f Calibration::getImageProjection(float aspectRatio, int screenRotation)
{
	return pimpl->getImageProjection(aspectRatio, screenRotation);
}
Matrix44f Calibration::getProjectionGL(const float nearPlane, const float farPlane, float aspectRatio, int screenRotation)
{
	return pimpl->getProjectionGL(nearPlane, farPlane, aspectRatio, screenRotation);
}




CppCamera::CppCamera()
{
	cv::setNumThreads(0);
	pimpl = new Camera_impl();
}
CppCamera::~CppCamera()
{
	delete pimpl;
	pimpl = nullptr;
}
Camera_impl* CppCamera::getImpl()
{
	return pimpl;
}
bool CppCamera::open(int camera)
{
	return pimpl->open(camera);
}
bool CppCamera::start()
{
	return pimpl->start();
}
bool CppCamera::stop()
{
	return pimpl->stop();
}
bool CppCamera::close()
{
	return pimpl->close();
}
Vector2i CppCamera::size()
{
	return pimpl->size();
}
int CppCamera::supportedSizeCount()
{
	return pimpl->supportedSizeCount();
}
Vector2i CppCamera::supportedSize(int index)
{
	return pimpl->supportedSize(index);
}
bool CppCamera::setSize(Vector2i size)
{
	return pimpl->setSize(size);
}
int CppCamera::supportedFrameRateRangeCount()
{
	return pimpl->supportedFrameRateRangeCount();
}
float CppCamera::supportedFrameRateRangeLower(int index)
{
	return pimpl->supportedFrameRateRangeLower(index);
}
float CppCamera::supportedFrameRateRangeUpper(int index)
{
	return pimpl->supportedFrameRateRangeUpper(index);
}
int CppCamera::frameRateRange()
{
	return pimpl->frameRateRange();
}
bool CppCamera::setFrameRateRange(int index)
{
	return pimpl->setFrameRateRange(index);
}
bool CppCamera::setFlashTorchMode(bool on)
{
	return pimpl->setFlashTorchMode(on);
}
bool CppCamera::setFocusMode(FocusMode focusMode)
{
	return pimpl->setFocusMode(focusMode);
}
bool CppCamera::autoFocus()
{
	return pimpl->autoFocus();
}
Calibration* CppCamera::cameraCalibration()
{
	return new Calibration(pimpl->cameraCalibration());
}
