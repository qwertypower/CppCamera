#include "Camera_impl_android.h"

#ifdef PLATFORM_ANDROID

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>

static JavaVM* jvm = NULL;
static jweak ctx = NULL;

JNIEnv* GetJniEnv() {
	JNIEnv* jni_env = 0;
	jvm->AttachCurrentThread(&jni_env, 0);
	return jni_env;
}

extern "C"
{
	JNIEXPORT void JNICALL Java_com_CppCamera_Engine_cppinitialize(JNIEnv* env, jobject thiz, jobject activity)
	{
		env->GetJavaVM(&jvm);
		ctx = GetJniEnv()->NewWeakGlobalRef(activity);
	}

	JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
		memset(&jvm, 0, sizeof(jvm));
		jvm = vm;
		return  JNI_VERSION_1_6;
	}
}

extern "C" JNIEXPORT void JNICALL onCameraFrame(JNIEnv * env, jobject thiz, jlong obj, jbyteArray data, jint data_size,
	jint pixel_format, jint width, jint height, jlong p6)
{
	Camera_impl& c = *(Camera_impl*)obj;
	if (c.onCameraFrameCallback == nullptr)
		return;

	jbyte* b = (jbyte*)env->GetByteArrayElements(data, NULL);
	c.onCameraFrameCallback((unsigned char*)b, (int)data_size, (int)width, (int)height, (int)pixel_format);
	env->ReleaseByteArrayElements(data, b, 0);
}

extern "C" JNIEXPORT void JNICALL onCameraFrame2(JNIEnv * env, jobject thiz, jlong obj, jobject data1, jobject data2,
	jobject data3, jint stride1, jint stride2, jint stride3, jint pixel_format, jint width, jint height, jint pixel_stride)
{
	Camera_impl& c = *(Camera_impl*)obj;
	if (c.onCameraFrameCallback2 == nullptr)
		return;

	jbyte* b1 = (jbyte*)env->GetDirectBufferAddress(data1);// env->GetByteArrayElements(data1, NULL);
	jbyte* b2 = (jbyte*)env->GetDirectBufferAddress(data2);// env->GetByteArrayElements(data2, NULL);
	jbyte* b3 = (jbyte*)env->GetDirectBufferAddress(data3);//env->GetByteArrayElements(data3, NULL);
	

	c.onCameraFrameCallback2((unsigned char*)b1, (unsigned char*)b2, (unsigned char*)b3, stride1, stride2, stride3, pixel_format, width, height, pixel_stride);

	/*env->ReleaseByteArrayElements(data1, b1, 0);
	env->ReleaseByteArrayElements(data2, b2, 0);
	env->ReleaseByteArrayElements(data3, b3, 0);*/
}


Calibration_impl::Calibration_impl()
{
	K = cv::Mat::zeros(3, 3, CV_32F);
}
Vector2i Calibration_impl::getSize()
{
	return Vector2i({ {{(int)width, (int)height}} });
}
Vector2f Calibration_impl::getFocalLength()
{
	return { {{0.f, 0.f}} };
}
Vector2f Calibration_impl::getPrincipalPoint()
{
	return { {{K.at<float>(0, 2), K.at<float>(1, 2)}} };
}
Vector4f Calibration_impl::getDistortionParameters()
{
	return { {{0.f, 0.f, 0.f, 0.f}} };
}
int Calibration_impl::getRotation()
{
	return rotation;
}
Matrix44f Calibration_impl::getImageProjection(float aspectRatio, int screenRotation)
{
	glm::mat4 m(0.0f);
	float imgAspect = width > height ? height / width : width / height;
	if (aspectRatio < 1.f) aspectRatio = 1.f / aspectRatio;
	m[1][0] = -1.f;
	m[0][1] = aspectRatio * imgAspect;
	m[2][2] = 1.f;
	m[3][3] = 1.f;

	if (frontal) m = glm::scale(m, glm::vec3(-1.0f, 1.0f, 1.0f));
	m = glm::rotate(m, glm::radians(270.f - screenRotation + rotation), glm::vec3(0.0f, 0.0f, 1.0f));
	Matrix44f out;
	memcpy(&out.data[0], &m[0][0], sizeof(float) * 16);
	return out;
}
Matrix44f Calibration_impl::getProjectionGL(const float nearPlane, const float farPlane, float aspectRatio, int screenRotation)
{
	Matrix44f out;
	glm::mat4 m(0.0f);
	float imgAspect = height / width;
	if (aspectRatio < 1.f)
	{
		aspectRatio = 1.f / aspectRatio;
		m[1][1] = 2.0 * K.at<float>(0, 0) / width;
		m[0][0] = 2.0 * K.at<float>(1, 1) / height * aspectRatio * imgAspect;
	}
	else
	{
		m[0][0] = 2.0 * K.at<float>(0, 0) / width;
		m[1][1] = 2.0 * K.at<float>(1, 1) / height * aspectRatio * imgAspect;
	}
	m[2][0] = 0.0; //2.0 * K.at<float>(0, 2) / width - 1.0;
	m[2][1] = 0.0; //2.0 * K.at<float>(1, 2) / height - 1.0;
	m[2][2] = -(farPlane + nearPlane) / (farPlane - nearPlane);
	m[2][3] = -1.0;
	m[3][2] = -2.0 * farPlane * nearPlane / (farPlane - nearPlane);

	m = glm::rotate(m, glm::radians((float)screenRotation - rotation), glm::vec3(0.0f, 0.0f, 1.0f));
	if (frontal) m = glm::scale(m, glm::vec3(1.0f, -1.0f, 1.0f));
	memcpy(&out.data[0], &m[0][0], sizeof(float) * 16);
	return out;
}
void Calibration_impl::calibrate(float fov, float fwidth, float fheight)
{
	if (diagFOV != fov || width != fwidth || height != fwidth)
	{
		diagFOV = fov;
		width = fwidth;
		height = fheight;

		float f;
		if (fov < 0.f)
			f = (1.7f * width) / 2.0f; // MAGIC HERE!!!!
		else
			f = (sqrtf(width * width + height * height) / tanf((diagFOV / 2.f) * CV_PI / 180.f)) / 2.f;

		float Cx = width / 2.f, Cy = height / 2.f;
		K.at<float>(0, 0) = f;
		K.at<float>(1, 1) = f;
		K.at<float>(0, 2) = Cx;
		K.at<float>(1, 2) = Cy;
		K.at<float>(2, 2) = 1.f;
	}
}


// CAMERA IMPLEMENTATION

Camera_impl::Camera_impl()
{
	camera_calibration = new Calibration_impl();

	JNIEnv* local_jni = GetJniEnv();
	jclass versionClass = local_jni->FindClass("android/os/Build$VERSION");
	jfieldID sdkIntFieldID = local_jni->GetStaticFieldID(versionClass, "SDK_INT", "I");
	android_os_api_level = 19;// local_jni->GetStaticIntField(versionClass, sdkIntFieldID);

	jclass clz;
	if (android_os_api_level < 21)
		clz = local_jni->FindClass("com/CppCamera/CppCameraDeviceAPI1");
	else
		clz = local_jni->FindClass("com/CppCamera/CppCameraDeviceAPI2");
	cam = (jclass)local_jni->NewGlobalRef(clz);
	jmethodID constr = local_jni->GetMethodID(cam, "<init>", "()V");
	jobject handler = local_jni->NewObject(cam, constr/*, (int64_t)(jlong*)this*/);
	camobj = local_jni->NewGlobalRef(handler);

	printf("[CppCamera] Creating camera object\n", 0);
	printf("[CppCamera] Running on API android-%d\n", android_os_api_level);
	printf("[CppCamera] Using android camera API%d\n", android_os_api_level >= 21 ? 2 : 1);
	fflush(stdout);

	// register native object
	jsetCameraDeviceRawPointer = local_jni->GetMethodID(cam, "setCameraDeviceRawPointer", "(J)V");
	local_jni->CallVoidMethod(camobj, jsetCameraDeviceRawPointer, (int64_t)(jlong*)this);

	// add jni callback
	if (android_os_api_level < 21)
	{
		JNINativeMethod methods[] = {
			{ "nativeCameraFrame", "(J[BIIIIJ)V", (void*)&onCameraFrame }
		};
		local_jni->RegisterNatives(cam, methods, 1);
	}
	else
	{
		JNINativeMethod methods[] = {
			{ "nativeCamera2Frame", "(JLjava/nio/ByteBuffer;Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;IIIIIII)V", (void*)& onCameraFrame2 }
		};
		local_jni->RegisterNatives(cam, methods, 1);
	}

	// register other methods
	jopen = local_jni->GetMethodID(cam, "open", "(Landroid/content/Context;I)Z");
	jstart = local_jni->GetMethodID(cam, "start", "()Z");
	jstopAndClose = local_jni->GetMethodID(cam, "stopAndClose", "()Z");
	jisCamera2 = local_jni->GetMethodID(cam, "isCamera2", "()Z");
	jisFrontFacing = local_jni->GetMethodID(cam, "isFrontFacing", "()Z");
	jgetCameraOrientation = local_jni->GetMethodID(cam, "getCameraOrientation", "()I");
	jgetSizeWidth = local_jni->GetMethodID(cam, "getSizeWidth", "()I");
	jgetSizeHeight = local_jni->GetMethodID(cam, "getSizeHeight", "()I");
	jgetNumSupportedSize = local_jni->GetMethodID(cam, "getNumSupportedSize", "()I");
	jgetSupportedSizeWidth = local_jni->GetMethodID(cam, "getSupportedSizeWidth", "(I)I");
	jgetSupportedSizeHeight = local_jni->GetMethodID(cam, "getSupportedSizeHeight", "(I)I");
	jsetSize = local_jni->GetMethodID(cam, "setSize", "(II)Z");
	jgetNumSupportedFrameRateRange = local_jni->GetMethodID(cam, "getNumSupportedFrameRateRange", "()I");
	jgetSupportedFrameRateRangeLower = local_jni->GetMethodID(cam, "getSupportedFrameRateRangeLower", "(I)F");
	jgetSupportedFrameRateRangeUpper = local_jni->GetMethodID(cam, "getSupportedFrameRateRangeUpper", "(I)F");
	jgetFrameRateRange = local_jni->GetMethodID(cam, "getFrameRateRange", "()I");
	jsetFrameRateRange = local_jni->GetMethodID(cam, "setFrameRateRange", "(I)Z");
	jsetFlashTorchMode = local_jni->GetMethodID(cam, "setFlashTorchMode", "(Z)Z");
	jsetFocusMode = local_jni->GetMethodID(cam, "setFocusMode", "(I)Z");
	jautoFocus = local_jni->GetMethodID(cam, "autoFocus", "()Z");
}
Camera_impl::~Camera_impl()
{
	JNIEnv* local_jni = GetJniEnv();
	local_jni->CallBooleanMethod(camobj, jstopAndClose);
	local_jni->DeleteGlobalRef(cam);
	local_jni->DeleteGlobalRef(camobj);
	printf("[CppCamera] Camera object destroyed\n", 0);
	fflush(stdout);
	delete camera_calibration;
	camera_calibration = nullptr;
}
bool Camera_impl::open(int camera)
{
	printf("[CppCamera] Camera.open(%d)\n", (int)camera);
	fflush(stdout);
	bool ret = GetJniEnv()->CallBooleanMethod(camobj, jopen, ctx, (jint)camera);
	return ret;
}
bool Camera_impl::start()
{
	JNIEnv* local_jni = GetJniEnv();
	bool ret = local_jni->CallBooleanMethod(camobj, jstart);
	if (ret)
	{
		preview_size = size();
		camera_calibration->rotation = local_jni->CallIntMethod(camobj, jgetCameraOrientation);
		camera_calibration->frontal = local_jni->CallBooleanMethod(camobj, jisFrontFacing);
		camera_calibration->calibrate(-1, preview_size.data[0], preview_size.data[1]);
		printf("[CppCamera] Camera.start()\n", 0);
	}
	return ret;
}
bool Camera_impl::stop()
{
	printf("[CppCamera] Camera.stop()\n", 0);
	fflush(stdout);
	bool ret = GetJniEnv()->CallBooleanMethod(camobj, jstopAndClose);
	return ret;
}
bool Camera_impl::close()
{
	printf("[CppCamera] Camera.close()\n", 0);
	fflush(stdout);
	bool ret = GetJniEnv()->CallBooleanMethod(camobj, jstopAndClose);
	return ret;
}
Vector2i Camera_impl::size()
{
	JNIEnv* local_jni = GetJniEnv();
	int w = local_jni->CallIntMethod(camobj, jgetSizeWidth);
	int h = local_jni->CallIntMethod(camobj, jgetSizeHeight);
	return { {{w, h}} };
}
int Camera_impl::supportedSizeCount()
{
	int ret = GetJniEnv()->CallIntMethod(camobj, jgetNumSupportedSize);
	return ret;
}
Vector2i Camera_impl::supportedSize(int idx)
{
	JNIEnv* local_jni = GetJniEnv();
	int w = local_jni->CallIntMethod(camobj, jgetSupportedSizeWidth);
	int h = local_jni->CallIntMethod(camobj, jgetSupportedSizeHeight);
	return { {{w, h}} };
}
bool Camera_impl::setSize(Vector2i size)
{
	bool ret = GetJniEnv()->CallBooleanMethod(camobj, jsetSize, (jint)size.data[0], (jint)size.data[1]);
	return ret;
}
int Camera_impl::supportedFrameRateRangeCount()
{
	int ret = GetJniEnv()->CallIntMethod(camobj, jgetNumSupportedFrameRateRange);
	return ret;
}
float Camera_impl::supportedFrameRateRangeLower(int index)
{
	float ret = GetJniEnv()->CallIntMethod(camobj, jgetSupportedFrameRateRangeLower, (jint)index);
	return ret;
}
float Camera_impl::supportedFrameRateRangeUpper(int index)
{
	float ret = GetJniEnv()->CallIntMethod(camobj, jgetSupportedFrameRateRangeUpper, (jint)index);
	return ret;
}
int Camera_impl::frameRateRange()
{
	int ret = GetJniEnv()->CallIntMethod(camobj, jgetFrameRateRange);
	return ret;
}
bool Camera_impl::setFrameRateRange(int index)
{
	bool ret = GetJniEnv()->CallBooleanMethod(camobj, jsetFrameRateRange, (jint)index);
	return ret;
}
bool Camera_impl::setFlashTorchMode(bool on)
{
	bool ret = GetJniEnv()->CallBooleanMethod(camobj, jsetFlashTorchMode, on);
	return ret;
}
bool Camera_impl::setFocusMode(FocusMode focusMode)
{
	bool ret = GetJniEnv()->CallBooleanMethod(camobj, jsetFocusMode, (jint)focusMode);
	return ret;
}
bool Camera_impl::autoFocus()
{
	bool ret = GetJniEnv()->CallBooleanMethod(camobj, jautoFocus);
	return ret;
}
Calibration_impl* Camera_impl::cameraCalibration()
{
	return camera_calibration;
}


#endif