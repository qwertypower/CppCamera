#include <jni.h>
#include <mutex>

#include "bridge.h"

std::mutex mtx;

#define JNIFUNC(clazz, sig) Java_com_julivi_cppcameraexample_##clazz##_##sig


extern "C"
{
	JNIEXPORT void JNIFUNC(CppCameraTest, switchCamera(JNIEnv* env, jobject thiz))
	{
		std::lock_guard<std::mutex> lock(mtx);
		switchCamera();
	}
	
	JNIEXPORT void JNIFUNC(CppCameraTest, enableFlashlight(JNIEnv* env, jobject thiz))
	{
		std::lock_guard<std::mutex> lock(mtx);
		enableFlashlight();
	}

	JNIEXPORT void JNIFUNC(GLView, nativeInitGL(JNIEnv* env, jobject thiz))
	{
		std::lock_guard<std::mutex> lock(mtx);
		initGL();
	}

	JNIEXPORT void JNIFUNC(GLView, nativeRender(JNIEnv* env, jobject thiz, jint width, jint height, jint rotation))
	{
		std::lock_guard<std::mutex> lock(mtx);
		render(width, height, rotation);
	}

	JNIEXPORT void JNIFUNC(GLView, nativeResume(JNIEnv* env, jobject thiz))
	{
		std::lock_guard<std::mutex> lock(mtx);
		onResume();
	}

	JNIEXPORT void JNIFUNC(GLView, nativePause(JNIEnv* env, jobject thiz))
	{
		std::lock_guard<std::mutex> lock(mtx);
		onPause();
	}

	JNIEXPORT void JNIFUNC(GLView, nativeDestroy(JNIEnv* env, jobject thiz))
	{
		std::lock_guard<std::mutex> lock(mtx);
		onDestroy();
	}
}