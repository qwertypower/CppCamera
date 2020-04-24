
package com.julivi.cppcameraexample;

import javax.microedition.khronos.egl.*;
import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.app.*;
import android.graphics.*;
import android.view.*;

public class GLView extends GLSurfaceView {
	private native void nativeInitGL();

	private native void nativeRender(int width, int height, int rotation);

	private native void nativeResume();

	private native void nativePause();

	private native void nativeDestroy();

	private int width = 1;
	private int height = 1;

	public GLView(Context context) {
		super(context);

		ActivityManager activityManager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
		android.content.pm.ConfigurationInfo configurationInfo = activityManager.getDeviceConfigurationInfo();
		boolean es3_support = (configurationInfo.reqGlEsVersion >= 0x30000);

		if (es3_support)
			setEGLContextClientVersion(3);
		else
			setEGLContextClientVersion(2);

		this.setEGLConfigChooser(8, 8, 8, 8, 24, 0);
		this.getHolder().setFormat(PixelFormat.RGBA_8888);
		this.setPreserveEGLContextOnPause(true);

		this.setRenderer(new GLSurfaceView.Renderer() {
			@Override
			public void onSurfaceCreated(GL10 gl, EGLConfig config) {
				nativeInitGL();
			}

			@Override
			public void onSurfaceChanged(GL10 gl, int w, int h) {
				width = w;
				height = h;
			}

			@Override
			public void onDrawFrame(GL10 gl) {
				nativeRender(width, height, GetScreenRotation());
			}
		});
		this.setZOrderMediaOverlay(true);
	}

	@Override
	protected void onAttachedToWindow() {
		super.onAttachedToWindow();
	}

	@Override
	protected void onDetachedFromWindow() {
		nativeDestroy();
		super.onDetachedFromWindow();
	}

	@Override
	public void onResume() {
		super.onResume();
		nativeResume();
	}

	@Override
	public void onPause() {
		nativePause();
		super.onPause();
	}

	private int GetScreenRotation() {
		int rotation = ((WindowManager) getContext().getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay()
				.getRotation();
		int orientation;
		switch (rotation) {
		case Surface.ROTATION_0:
			orientation = 0;
			break;
		case Surface.ROTATION_90:
			orientation = 90;
			break;
		case Surface.ROTATION_180:
			orientation = 180;
			break;
		case Surface.ROTATION_270:
			orientation = 270;
			break;
		default:
			orientation = 0;
			break;
		}
		return orientation;
	}
}