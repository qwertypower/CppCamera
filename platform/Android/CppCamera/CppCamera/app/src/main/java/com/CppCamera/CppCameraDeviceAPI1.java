
package com.CppCamera;

import android.hardware.*;
import android.graphics.SurfaceTexture;
import android.content.*;
import android.util.*;
import java.util.*;

class CppCameraDeviceAPI1 implements Camera.PreviewCallback {
	private int mCameraId;
	private int mCameraDeviceId;
	private volatile Camera mCamera;
	private boolean mIsFrontFacing;
	private SurfaceTexture mSurfaceTexture;
	private List<Camera.Size> mSupportedPreviewSizes;
	private List<int[]> mSupportedPreviewFpsRanges;
	private int mCurrentFpsRange;
	private int mCameraRotation;
	private int userMode;
	private int previousMode;
	private boolean mStarted;
	private int mCachedWidth;
	private int mCachedHeight;
	private int mPixelFormat;
	private int setWhenStopFocus;
	private boolean setWhenStopFlash;
	private long mCameraDeviceRawPointer;
	private final Object mCameraDeviceRawPointerLock;
	Camera.AutoFocusCallback autoFacusCallback;

	CppCameraDeviceAPI1() {
		this.mCameraDeviceId = -1;
		this.mIsFrontFacing = false;
		this.mSurfaceTexture = new SurfaceTexture(0);
		this.mCurrentFpsRange = -1;
		this.userMode = -1;
		this.previousMode = -1;
		this.mStarted = false;
		this.mCachedWidth = 1280;
		this.mCachedHeight = 720;
		this.mPixelFormat = 0;
		this.setWhenStopFocus = -1;
		this.setWhenStopFlash = false;
		this.mCameraDeviceRawPointer = 0L;
		this.mCameraDeviceRawPointerLock = new Object();
		this.autoFacusCallback = (Camera.AutoFocusCallback) new Camera.AutoFocusCallback() {
			public void onAutoFocus(final boolean success, final Camera camera) {
				if (CppCameraDeviceAPI1.this.userMode >= 0) {
					CppCameraDeviceAPI1.this.setFocusMode(CppCameraDeviceAPI1.this.userMode);
				}
				CppCameraDeviceAPI1.this.userMode = -1;
			}
		};
	}

	public boolean open(final Context context, final int id) {
		boolean opened = false;
		this.stopPreviewAndRelease();
		if (id == 0) {
			this.mCameraId = 0;
			opened = this.openWithIndex(this.mCameraId);
		} else if (id == 1) {
			final Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
			for (int cameraCount = Camera.getNumberOfCameras(), camIdx = 0; camIdx < cameraCount; ++camIdx) {
				Camera.getCameraInfo(camIdx, cameraInfo);
				if (cameraInfo.facing == 0) {
					this.mCameraId = camIdx;
					opened = this.openWithIndex(this.mCameraId);
					break;
				}
			}
		} else if (id == 2) {
			final Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
			for (int cameraCount = Camera.getNumberOfCameras(), camIdx = 0; camIdx < cameraCount; ++camIdx) {
				Camera.getCameraInfo(camIdx, cameraInfo);
				if (cameraInfo.facing == 1) {
					this.mCameraId = camIdx;
					opened = this.openWithIndex(this.mCameraId);
					break;
				}
			}
		}
		if (opened) {
			this.mCameraDeviceId = id;
			final Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
			Camera.getCameraInfo(this.mCameraId, cameraInfo);
			this.mIsFrontFacing = (cameraInfo.facing == 1);
		}
		return opened;
	}

	private boolean openWithIndex(final int index) {
		try {
			this.stopPreviewAndRelease();
			this.mCamera = Camera.open(index);
			if (this.mCamera != null) {
				final Camera.Parameters params = this.mCamera.getParameters();
				final int format = params.getPreviewFormat();
				switch (format) {
				case 17: {
					this.mPixelFormat = 2;
					break;
				}
				case 842094169: {
					this.mPixelFormat = 3;
					break;
				}
				default: {
					this.mPixelFormat = 0;
					break;
				}
				}
				this.mSupportedPreviewSizes = (List<Camera.Size>) params.getSupportedPreviewSizes();
				this.mSupportedPreviewFpsRanges = (List<int[]>) params.getSupportedPreviewFpsRange();
				if (this.mCurrentFpsRange < 0 || this.mCurrentFpsRange >= this.mSupportedPreviewFpsRanges.size()) {
					this.mCurrentFpsRange = this.mSupportedPreviewFpsRanges.size() - 1;
					int i = 0;
					for (final int[] range : this.mSupportedPreviewFpsRanges) {
						if (range[0] == 30000 && range[1] == 30000) {
							this.mCurrentFpsRange = i;
						}
						++i;
					}
				}
				params.setPreviewFpsRange(this.mSupportedPreviewFpsRanges.get(this.mCurrentFpsRange)[0],
						this.mSupportedPreviewFpsRanges.get(this.mCurrentFpsRange)[1]);
				this.mCamera.setParameters(params);
				this.setSize(this.mCachedWidth, this.mCachedHeight);
				return true;
			}
		} catch (Exception e) {
			Log.e("CppCamera", "Camera1: \n" + Log.getStackTraceString((Throwable) e));
			if (this.mCamera != null) {
				try {
					this.mCamera.release();
				} catch (Exception ex) {
				}
				this.mCamera = null;
			}
			return false;
		}
		return false;
	}

	public boolean isFrontFacing() {
		return this.mIsFrontFacing;
	}

	public boolean ready() {
		return this.mCamera != null;
	}

	private float getRatioError(final float x, final float x0) {
		final float a = x / x0 - 1.0f;
		final float b = x0 / x - 1.0f;
		return a * a + b * b;
	}

	private Camera.Size getOptimalPreviewSize(final int width, final int height) {
		final long area = width * height;
		Camera.Size res = this.mSupportedPreviewSizes.get(0);
		float minError = Float.MAX_VALUE;
		for (final Camera.Size size : this.mSupportedPreviewSizes) {
			final float error = this.getRatioError((float) width, (float) size.width)
					+ this.getRatioError((float) height, (float) size.height);
			if (error < minError) {
				minError = error;
				res = size;
			}
		}
		return res;
	}

	public boolean start() {
		if (!this.ready() && !this.open(null, this.mCameraId)) {
			return false;
		}
		try {
			this.mCamera.setPreviewTexture(this.mSurfaceTexture);
			this.mCamera.setPreviewCallbackWithBuffer((Camera.PreviewCallback) this);
			for (int i = 0; i < 2; ++i) {
				this.mCamera.addCallbackBuffer(new byte[this.mCachedWidth * this.mCachedHeight * 3 / 2]);
			}
			this.mCamera.startPreview();
		} catch (Exception e) {
			Log.e("CppCamera", "Camera1: \n" + Log.getStackTraceString((Throwable) e));
			return false;
		}
		this.setCameraRotation();
		if (this.previousMode >= 0) {
			this.setFocusMode(this.previousMode);
		}
		if (this.setWhenStopFocus != -1) {
			this.setFocusMode(this.setWhenStopFocus);
			this.setWhenStopFocus = -1;
		}
		if (this.setWhenStopFlash) {
			this.setFlashTorchMode(this.setWhenStopFlash);
			this.setWhenStopFlash = false;
		}
		return this.mStarted = true;
	}

	public boolean stopAndClose() {
		this.stopPreviewAndRelease();
		return true;
	}

	private void stopPreviewAndRelease() {
		if (!this.ready()) {
			return;
		}
		this.mStarted = false;
		try {
			this.mCamera.stopPreview();
		} catch (Exception ex) {
		}
		try {
			this.mCamera.setPreviewCallback((Camera.PreviewCallback) null);
		} catch (Exception ex2) {
		}
		try {
			this.mCamera.release();
		} catch (Exception ex3) {
		}
		this.mCamera = null;
	}

	private void setCameraRotation() {
		if (!this.ready()) {
			return;
		}
		try {
			final Camera.Parameters params = this.mCamera.getParameters();
			params.setRotation(this.mCameraRotation);
			this.mCamera.setParameters(params);
		} catch (Exception e) {
			Log.e("CppCamera", "Camera1: \n" + Log.getStackTraceString((Throwable) e));
		}
	}

	public Camera.CameraInfo getCameraInfo() {
		final Camera.CameraInfo info = new Camera.CameraInfo();
		Camera.getCameraInfo(this.mCameraId, info);
		return info;
	}

	public int getCameraRotation() {
		return this.mCameraRotation;
	}

	public int getCameraOrientation() {
		return this.getCameraInfo().orientation;
	}

	public void onPreviewFrame(final byte[] data, final Camera camera) {
		if (data == null) {
			return;
		}
		synchronized (this.mCameraDeviceRawPointerLock) {
			if (this.mCameraDeviceRawPointer != 0L) {
				nativeCameraFrame(this.mCameraDeviceRawPointer, data, data.length, this.mPixelFormat, this.mCachedWidth,
						this.mCachedHeight, 0L);
			}
		}
		camera.addCallbackBuffer(data);
	}

	public int getNumSupportedFrameRateRange() {
		if (!this.ready()) {
			return 0;
		}
		return this.mSupportedPreviewFpsRanges.size();
	}

	public float getSupportedFrameRateRangeLower(final int index) {
		if (this.mSupportedPreviewFpsRanges == null) {
			return 0.0f;
		}
		if (index < 0 || index >= this.mSupportedPreviewFpsRanges.size()) {
			return 0.0f;
		}
		return this.mSupportedPreviewFpsRanges.get(index)[0] / 1000.0f;
	}

	public float getSupportedFrameRateRangeUpper(final int index) {
		if (this.mSupportedPreviewFpsRanges == null) {
			return 0.0f;
		}
		if (index < 0 || index >= this.mSupportedPreviewFpsRanges.size()) {
			return 0.0f;
		}
		return this.mSupportedPreviewFpsRanges.get(index)[1] / 1000.0f;
	}

	public int getFrameRateRange() {
		if (this.mSupportedPreviewFpsRanges == null) {
			return -1;
		}
		return this.mCurrentFpsRange;
	}

	public boolean setFrameRateRange(final int index) {
		if (this.mSupportedPreviewFpsRanges == null) {
			return false;
		}
		if (index < 0 || index >= this.mSupportedPreviewFpsRanges.size()) {
			return false;
		}
		this.mCurrentFpsRange = index;
		return true;
	}

	public int getSizeWidth() {
		return this.mCachedWidth;
	}

	public int getSizeHeight() {
		return this.mCachedHeight;
	}

	public int getNumSupportedSize() {
		if (!this.ready()) {
			return 0;
		}
		return this.mSupportedPreviewSizes.size();
	}

	public int getSupportedSizeWidth(final int idx) {
		if (!this.ready() || idx >= this.mSupportedPreviewSizes.size()) {
			return 0;
		}
		final Camera.Size size = this.mSupportedPreviewSizes.get(idx);
		return size.width;
	}

	public int getSupportedSizeHeight(final int idx) {
		if (!this.ready() || idx >= this.mSupportedPreviewSizes.size()) {
			return 0;
		}
		final Camera.Size size = this.mSupportedPreviewSizes.get(idx);
		return size.height;
	}

	public boolean setSize(final int width, final int height) {
		if (!this.ready()) {
			return false;
		}
		try {
			if (this.mStarted) {
				this.mCamera.stopPreview();
				this.mCamera.setPreviewCallbackWithBuffer((Camera.PreviewCallback) null);
				this.mCamera.setPreviewCallbackWithBuffer((Camera.PreviewCallback) this);
			}
			Camera.Parameters params = this.mCamera.getParameters();
			Camera.Size size = this.getOptimalPreviewSize(width, height);
			params.setPreviewSize(size.width, size.height);
			this.mCamera.setParameters(params);
			params = this.mCamera.getParameters();
			size = params.getPreviewSize();
			this.mCachedWidth = size.width;
			this.mCachedHeight = size.height;
			if (this.mStarted) {
				for (int i = 0; i < 2; ++i) {
					this.mCamera.addCallbackBuffer(new byte[this.mCachedWidth * this.mCachedHeight * 3 / 2]);
				}
				this.mCamera.startPreview();
			}
			return true;
		} catch (Exception e) {
			Log.e("CppCamera", "Camera1: \n" + Log.getStackTraceString((Throwable) e));
			return false;
		}
	}

	public boolean setFlashTorchMode(final boolean on) {
		if (!this.ready()) {
			this.setWhenStopFlash = on;
			return false;
		}
		try {
			final Camera.Parameters params = this.mCamera.getParameters();
			String mode = null;
			if (on) {
				mode = "torch";
			} else {
				mode = "off";
			}
			if (this.isFlashModeSupported(mode)) {
				params.setFlashMode(mode);
				this.mCamera.setParameters(params);
				return true;
			}
			return false;
		} catch (Exception e) {
			Log.e("CppCamera", "Camera1: \n" + Log.getStackTraceString((Throwable) e));
			return false;
		}
	}

	private boolean isFlashModeSupported(final String mode) {
		if (!this.ready() || mode == null) {
			return false;
		}
		final List<String> supported = (List<String>) this.mCamera.getParameters().getSupportedFlashModes();
		return supported != null && supported.contains(mode);
	}

	private boolean checkFocusModeSupport(final int focusMode) {
		final Camera.Parameters params = this.mCamera.getParameters();
		final List<String> modes = (List<String>) params.getSupportedFocusModes();
		switch (focusMode) {
		case 0: {
			return modes.contains("auto");
		}
		case 2: {
			return modes.contains("continuous-picture") || modes.contains("continuous-video");
		}
		case 3: {
			return modes.contains("infinity");
		}
		case 4: {
			return modes.contains("macro");
		}
		case 5: {
			return false;
		}
		default: {
			return false;
		}
		}
	}

	public boolean setFocusMode(final int focusMode) {
		if (!this.ready()) {
			this.setWhenStopFocus = focusMode;
			return false;
		}
		try {
			if (this.userMode == -1) {
				final Camera.Parameters params = this.mCamera.getParameters();
				final List<String> modes = (List<String>) params.getSupportedFocusModes();
				switch (focusMode) {
				case 0: {
					if (modes.contains("auto")) {
						params.setFocusMode("auto");
						break;
					}
					return false;
				}
				case 2: {
					if (modes.contains("continuous-picture")) {
						params.setFocusMode("continuous-picture");
						break;
					}
					if (modes.contains("continuous-video")) {
						params.setFocusMode("continuous-video");
						break;
					}
					return false;
				}
				case 3: {
					if (modes.contains("infinity")) {
						params.setFocusMode("infinity");
						break;
					}
					return false;
				}
				case 4: {
					if (modes.contains("macro")) {
						params.setFocusMode("macro");
						break;
					}
					return false;
				}
				case 5: {
					return false;
				}
				default: {
					return false;
				}
				}
				this.previousMode = focusMode;
				this.mCamera.setParameters(params);
				return true;
			}
			if (!this.checkFocusModeSupport(focusMode)) {
				return false;
			}
			this.userMode = focusMode;
			return true;
		} catch (Exception e) {
			Log.e("CppCamera", "Camera1: \n" + Log.getStackTraceString((Throwable) e));
			return false;
		}
	}

	public void setCameraDeviceRawPointer(final long ptr) {
		synchronized (this.mCameraDeviceRawPointerLock) {
			this.mCameraDeviceRawPointer = ptr;
		}
	}

	public boolean isCamera2() {
		return false;
	}

	public boolean autoFocus() {
		try {
			this.mCamera.autoFocus(this.autoFacusCallback);
		} catch (RuntimeException e) {
			e.printStackTrace();
			Log.e("CppCamera", "Camera1: autoFocus must call after startPreview and in appropriate focus mode");
			return false;
		} catch (Exception e2) {
			Log.e("CppCamera", "Camera1: \n" + Log.getStackTraceString((Throwable) e2));
			return false;
		}
		return true;
	}

	private static native void nativeCameraFrame(final long camera_ptr, final byte[] data, final int data_size,
			final int pixel_format, final int width, final int height, final long p6);
}