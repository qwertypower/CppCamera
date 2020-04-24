package com.CppCamera;

import android.content.*;
import android.graphics.*;
import android.util.*;
import android.hardware.camera2.params.*;
import android.annotation.*;
import android.media.*;
import java.util.*;
import android.os.*;
import android.hardware.camera2.*;
import java.nio.*;
import java.util.concurrent.locks.*;
import android.widget.*;

@TargetApi(21)
class CppCameraDeviceAPI2 {
	private Context mContext;
	private String mCameraId;
	private int mCameraIndex;
	private boolean mIsTimeStampSourceRealTime;
	private boolean mIsTimeStampSystemRealTime;
	private long mTimeStampOffset;
	private long mL1;
	private boolean mIsFrontFacing;
	private CameraCharacteristics mCharacteristics;
	private int mSensorOrientation;
	private Size[] mSupportedSizes;
	private Rect mCameraActiveSize;
	private float mCurrentZoom;
	private Range<Integer>[] mSupportedFpsRanges;
	private int mCurrentFpsRange;
	private int mCurrentFocusMode;
	private boolean mFlashSupported;
	private boolean mFlashEnabled;
	private HandlerThread mBackgroundThread;
	private Handler mBackgroundHandler;
	private CameraDevice mCameraDevice;
	private ImageReader mImageReader;
	private CameraCaptureSession mCaptureSession;
	private final Object mCameraDeviceRawPointerLock;
	private long mCameraDeviceRawPointer;
	private boolean mIsStarted;
	private Size mCameraSize;

	CppCameraDeviceAPI2() {
		this.mIsTimeStampSourceRealTime = true;
		this.mIsTimeStampSystemRealTime = false;
		this.mTimeStampOffset = 0L;
		this.mIsFrontFacing = false;
		this.mSensorOrientation = 0;
		this.mCurrentZoom = 1.0f;
		this.mCurrentFpsRange = -1;
		this.mCurrentFocusMode = 0;
		this.mFlashSupported = false;
		this.mFlashEnabled = false;
		this.mCameraDeviceRawPointerLock = new Object();
		this.mCameraDeviceRawPointer = 0L;
		this.mIsStarted = false;
	}

	public void setCameraDeviceRawPointer(final long ptr) {
		synchronized (this.mCameraDeviceRawPointerLock) {
			this.mCameraDeviceRawPointer = ptr;
		}
	}

	@SuppressLint({ "MissingPermission" })
	public boolean open(final Context context, final int id) {
		if (context == null) {
			return false;
		}
		this.stopAndClose();
		boolean success = false;
		try {
			final boolean first = this.mContext == null;
			this.mContext = context;
			final CameraManager manager = (CameraManager) context.getSystemService("camera");

			if (manager == null) {
				return false;
			}

			boolean found = false;
			try {
				final String[] ids = manager.getCameraIdList();
				int indexBegin = 0;
				int indexEnd = ids.length;

				for (int index = indexBegin; index < indexEnd; ++index) {
					final String cameraId = ids[index];
					final CameraCharacteristics characteristics = manager.getCameraCharacteristics(cameraId);

					final Integer facing = (Integer) characteristics.get(CameraCharacteristics.LENS_FACING);

					if (id == 1 && facing != 1) {
						continue;
					}
					if (id == 2 && facing != 0) {
						continue;
					}

					if (characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP) != null) {
						this.mCameraId = cameraId;
						this.mCameraIndex = index;
						this.mCharacteristics = characteristics;
						found = true;
						break;
					}
				}
			} catch (CameraAccessException e) {
				Log.e("CppCamera", "Camera2: \n" + Log.getStackTraceString((Throwable) e));
				return false;
			} catch (Exception e2) {
				Log.e("CppCamera", "Camera2: \n" + Log.getStackTraceString((Throwable) e2));
				return false;
			}
			if (!found) {
				return false;
			}
			this.mIsTimeStampSourceRealTime = ((int) this.mCharacteristics
					.get(CameraCharacteristics.SENSOR_INFO_TIMESTAMP_SOURCE) == 1);
			if (first && !this.mIsTimeStampSourceRealTime) {
				Log.w("CppCamera", "Camera2: Timestamp is not precise.");
			}
			this.mIsFrontFacing = ((int) this.mCharacteristics.get(CameraCharacteristics.LENS_FACING) == 0);
			this.mSensorOrientation = (int) this.mCharacteristics.get(CameraCharacteristics.SENSOR_ORIENTATION);
			this.mSupportedSizes = ((StreamConfigurationMap) this.mCharacteristics
					.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)).getOutputSizes(35);
			this.mCameraActiveSize = (Rect) this.mCharacteristics
					.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE);
			this.mSupportedFpsRanges = (Range<Integer>[]) this.mCharacteristics
					.get(CameraCharacteristics.CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES);
			this.mFlashSupported = (boolean) this.mCharacteristics.get(CameraCharacteristics.FLASH_INFO_AVAILABLE);
			if (this.mCameraSize == null) {
				this.mCameraSize = this.getOptimalPreviewSize(1280, 720);
			}

			if (this.mCurrentFpsRange < 0 || this.mCurrentFpsRange >= this.mSupportedFpsRanges.length) {
				this.mCurrentFpsRange = this.mSupportedFpsRanges.length - 1;
				int index2 = 0;
				for (final Range<Integer> range : this.mSupportedFpsRanges) {
					if ((int) range.getLower() == 30 && (int) range.getUpper() == 30) {
						this.mCurrentFpsRange = index2;
					}
					++index2;
				}
			}
			(this.mBackgroundThread = new HandlerThread("CameraBackground")).start();
			this.mBackgroundHandler = new Handler(this.mBackgroundThread.getLooper());
			final AutoResetEvent signal = new AutoResetEvent();
			try {
				manager.openCamera(this.mCameraId, (CameraDevice.StateCallback) new CameraDevice.StateCallback() {
					public void onOpened(final CameraDevice cameraDevice) {
						CppCameraDeviceAPI2.this.mCameraDevice = cameraDevice;
						signal.set();
					}

					public void onDisconnected(final CameraDevice cameraDevice) {
						cameraDevice.close();
						signal.set();
					}

					public void onError(final CameraDevice cameraDevice, final int error) {
						cameraDevice.close();
						signal.set();
						Log.e("CppCamera", "Camera2: CameraManager.openCamera onError(" + error + ")\n");
					}
				}, this.mBackgroundHandler);
			} catch (CameraAccessException e3) {
				Log.e("CppCamera", "Camera2: \n" + Log.getStackTraceString((Throwable) e3));
				return false;
			} catch (Exception e4) {
				Log.e("CppCamera", "Camera2: \n" + Log.getStackTraceString((Throwable) e4));
				return false;
			}
			signal.waitOne();
			if (this.mCameraDevice == null) {
				return false;
			}
			success = true;
		} finally {
			if (!success) {
				this.stopAndClose();
			}
		}
		return success;
	}

	public boolean start() {
		if (this.mBackgroundThread == null || this.mBackgroundHandler == null || this.mCameraDevice == null) {
			if (this.mContext == null) {
				return false;
			}
			if (!this.open(this.mContext, this.mCameraIndex)) {
				return false;
			}
		}
		if (this.mIsStarted) {
			return true;
		}
		if (this.mCameraSize == null) {
			return false;
		}
		(this.mImageReader = ImageReader.newInstance(this.mCameraSize.getWidth(), this.mCameraSize.getHeight(),
				ImageFormat.YUV_420_888, 2)).setOnImageAvailableListener(
						(ImageReader.OnImageAvailableListener) new ImageReader.OnImageAvailableListener() {
							private boolean notSupportedFired = false;

							public void onImageAvailable(final ImageReader reader) {
								final Image image = reader.acquireLatestImage();
								if (image == null) {
									return;
								}
								try {
									int pixelFormat = 0;
									if (image.getPlanes().length == 3) {
										pixelFormat = 2;//checkPixelFormat(image.getPlanes()[0], image.getPlanes()[1], image.getPlanes()[2]);
									}
									if (pixelFormat == 0) {
										if (!this.notSupportedFired) {
											Log.e("CppCamera", "Camera2: Not supported image format.");
											this.notSupportedFired = true;
										}
										return;
									}
									synchronized (CppCameraDeviceAPI2.this.mCameraDeviceRawPointerLock) {
										if (CppCameraDeviceAPI2.this.mCameraDeviceRawPointer != 0L) {
											final Image.Plane[] planes = image.getPlanes();
											ByteBuffer buffer1 = planes[0].getBuffer();
											ByteBuffer buffer2 = planes[1].getBuffer();
											ByteBuffer buffer3 = planes[2].getBuffer();
											/*byte[] p1 = new byte[buffer1.remaining()];
											byte[] p2 = new byte[buffer2.remaining()];
											byte[] p3 = new byte[buffer3.remaining()];
											
											buffer1.get(p1);
											buffer2.get(p2);
											buffer3.get(p3);
											
											nativeCamera2Frame(CppCameraDeviceAPI2.this.mCameraDeviceRawPointer,
													p1, p2, p3,
													planes[0].getRowStride(), planes[1].getRowStride(),
													planes[2].getRowStride(), pixelFormat, image.getWidth(),
													image.getHeight(),
													planes[1].getPixelStride());*/

											nativeCamera2Frame(CppCameraDeviceAPI2.this.mCameraDeviceRawPointer, buffer1, buffer2,
													buffer3, planes[0].getRowStride(), planes[1].getRowStride(),
													planes[2].getRowStride(), pixelFormat, image.getWidth(),
													image.getHeight(), planes[1].getPixelStride());
										}
									}
								} finally {
									image.close();
								}
							}
						}, this.mBackgroundHandler);
		final AutoResetEvent signal = new AutoResetEvent();
		try {
			this.mCameraDevice.createCaptureSession((List) Collections.singletonList(this.mImageReader.getSurface()),
					(CameraCaptureSession.StateCallback) new CameraCaptureSession.StateCallback() {
						public void onConfigured(final CameraCaptureSession cameraCaptureSession) {
							CppCameraDeviceAPI2.this.mCaptureSession = cameraCaptureSession;
							signal.set();
						}

						public void onConfigureFailed(final CameraCaptureSession cameraCaptureSession) {
							Log.e("CppCamera", "Camera2: createCaptureSession onConfigureFailed!");
							signal.set();
						}

						public void onClosed(final CameraCaptureSession session) {
						}
					}, this.mBackgroundHandler);
		} catch (CameraAccessException e) {
			Log.e("CppCamera", "Camera2: \n" + Log.getStackTraceString((Throwable) e));
			return false;
		} catch (Exception e2) {
			Log.e("CppCamera", "Camera2: \n" + Log.getStackTraceString((Throwable) e2));
			return false;
		}
		signal.waitOne();
		if (this.mCaptureSession == null) {
			this.stopAndClose();
			return false;
		}
		if (!this.runRequest()) {
			this.stopAndClose();
			return false;
		}
		return this.mIsStarted = true;
	}

	public boolean stopAndClose() {
		this.mIsStarted = false;
		if (this.mCaptureSession != null) {
			try {
				this.mCaptureSession.stopRepeating();
			} catch (CameraAccessException ex) {
			} catch (Exception ex2) {
			}
			try {
				this.mCaptureSession.abortCaptures();
			} catch (CameraAccessException ex3) {
			} catch (Exception ex4) {
			}
			this.mCaptureSession = null;
		}
		if (this.mImageReader != null) {
			this.mImageReader.close();
			this.mImageReader = null;
		}
		if (this.mCameraDevice != null) {
			this.mCameraDevice.close();
			this.mCameraDevice = null;
		}
		if (this.mBackgroundThread != null) {
			this.mBackgroundThread.quitSafely();
			try {
				this.mBackgroundThread.join();
			} catch (InterruptedException ex5) {
			}
			this.mBackgroundThread = null;
			this.mBackgroundHandler = null;
		}
		return true;
	}

	public boolean isCamera2() {
		return true;
	}

	public boolean isFrontFacing() {
		return this.mIsFrontFacing;
	}

	public int getCameraOrientation() {
		return this.mSensorOrientation;
	}

	public int getSizeWidth() {
		if (this.mCameraSize == null) {
			return 0;
		}
		return this.mCameraSize.getWidth();
	}

	public int getSizeHeight() {
		if (this.mCameraSize == null) {
			return 0;
		}
		return this.mCameraSize.getHeight();
	}

	public boolean setSize(final int width, final int height) {
		if (this.mSupportedSizes == null) {
			return false;
		}
		this.mCameraSize = this.getOptimalPreviewSize(width, height);
		if (this.mIsStarted) {
			this.stopAndClose();
			this.start();
		}
		return true;
	}

	public int getNumSupportedSize() {
		if (this.mSupportedSizes == null) {
			return 0;
		}
		return this.mSupportedSizes.length;
	}

	public int getSupportedSizeWidth(final int index) {
		if (this.mSupportedSizes == null || index < 0 || index >= this.mSupportedSizes.length) {
			return 0;
		}
		return this.mSupportedSizes[index].getWidth();
	}

	public int getSupportedSizeHeight(final int index) {
		if (this.mSupportedSizes == null || index < 0 || index >= this.mSupportedSizes.length) {
			return 0;
		}
		return this.mSupportedSizes[index].getWidth();
	}

	public int getNumSupportedFrameRateRange() {
		if (this.mSupportedFpsRanges == null) {
			return 0;
		}
		return this.mSupportedFpsRanges.length;
	}

	public float getSupportedFrameRateRangeLower(final int index) {
		if (this.mSupportedFpsRanges == null) {
			return 0.0f;
		}
		if (index < 0 || index >= this.mSupportedFpsRanges.length) {
			return 0.0f;
		}
		return (float) (int) this.mSupportedFpsRanges[index].getLower();
	}

	public float getSupportedFrameRateRangeUpper(final int index) {
		if (this.mSupportedFpsRanges == null) {
			return 0.0f;
		}
		if (index < 0 || index >= this.mSupportedFpsRanges.length) {
			return 0.0f;
		}
		return (float) (int) this.mSupportedFpsRanges[index].getUpper();
	}

	public int getFrameRateRange() {
		if (this.mSupportedFpsRanges == null) {
			return -1;
		}
		return this.mCurrentFpsRange;
	}

	public boolean setFrameRateRange(final int index) {
		if (this.mSupportedFpsRanges == null) {
			return false;
		}
		if (index < 0 || index >= this.mSupportedFpsRanges.length) {
			return false;
		}
		this.mCurrentFpsRange = index;
		if (this.mIsStarted) {
			this.runRequest();
		}
		return true;
	}

	public boolean setFlashTorchMode(final boolean on) {
		if (!this.mFlashSupported) {
			return false;
		}
		this.mFlashEnabled = on;
		if (this.mIsStarted) {
			this.runRequest();
		}
		return true;
	}

	public boolean setFocusMode(final int focusMode) {
		if (this.mCharacteristics == null) {
			return false;
		}
		if (focusMode != 0) {
			if (focusMode == 2) {
				if (!contains((int[]) this.mCharacteristics.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES),
						4)) {
					if (!contains(
							(int[]) this.mCharacteristics.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES),
							3)) {
						Log.e("CppCamera",
								String.format("Camera2: FocusMode.Continousauto not supported", new Object[0]));
						return false;
					}
				}
			} else if (focusMode == 3) {
				if (!contains((int[]) this.mCharacteristics.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES),
						1)) {
					Log.e("CppCamera", String.format(
							"Camera2: FocusMode.Infinity not supported: manual sensor not available", new Object[0]));
					return false;
				}
			} else if (focusMode != 4) {
				if (focusMode != 5) {
					return false;
				}
				if (!contains((int[]) this.mCharacteristics.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES),
						1)) {
					Log.e("CppCamera", String.format(
							"Camera2: FocusMode.Medium not supported: manual sensor not available", new Object[0]));
					return false;
				}
			}
		}
		this.mCurrentFocusMode = focusMode;
		if (this.mIsStarted) {
			this.runRequest();
		}
		return true;
	}

	public boolean autoFocus() {
		this.runRequest(false);
		this.runRequest(true);
		return true;
	}

	private boolean runRequest() {
		return this.runRequest(true);
	}

	private boolean runRequest(final boolean repeating) {
		if (this.mCameraDevice == null) {
			return false;
		}
		if (this.mCaptureSession == null) {
			return false;
		}
		CaptureRequest.Builder builder;
		try {
			builder = this.mCameraDevice.createCaptureRequest(1);
		} catch (CameraAccessException e) {
			Log.e("CppCamera", "Camera2: \n" + Log.getStackTraceString((Throwable) e));
			return false;
		} catch (Exception e2) {
			Log.e("CppCamera", "Camera2: \n" + Log.getStackTraceString((Throwable) e2));
			return false;
		}
		builder.addTarget(this.mImageReader.getSurface());
		final int newWidth = (int) Math.round(Math.ceil(this.mCameraActiveSize.width() / (this.mCurrentZoom * 8.0f)))
				* 8;
		final int newHeight = (int) Math.round(Math.ceil(this.mCameraActiveSize.height() / (this.mCurrentZoom * 8.0f)))
				* 8;
		final int left = this.mCameraActiveSize.left + (this.mCameraActiveSize.width() - newWidth) / 2;
		final int top = this.mCameraActiveSize.top + (this.mCameraActiveSize.height() - newHeight) / 2;
		final int right = left + newWidth;
		final int bottom = top + newHeight;
		builder.set(CaptureRequest.SCALER_CROP_REGION, new Rect(left, top, right, bottom));
		builder.set(CaptureRequest.CONTROL_AE_MODE, 1);
		builder.set(CaptureRequest.STATISTICS_FACE_DETECT_MODE, 0);
		builder.set(CaptureRequest.CONTROL_VIDEO_STABILIZATION_MODE, 0);
		builder.set(CaptureRequest.LENS_OPTICAL_STABILIZATION_MODE, 0);
		builder.set(CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE, this.mSupportedFpsRanges[this.mCurrentFpsRange]);
		if (this.mFlashEnabled) {
			builder.set(CaptureRequest.FLASH_MODE, 2);
		}
		builder.set(CaptureRequest.CONTROL_MODE, 1);
		if (this.mCurrentFocusMode == 0) {
			builder.set(CaptureRequest.CONTROL_AF_MODE, 1);
			if (!repeating) {
				builder.set(CaptureRequest.CONTROL_AF_TRIGGER, 1);
			}
		} else if (this.mCurrentFocusMode == 2) {
			if (contains((int[]) this.mCharacteristics.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES), 4)) {
				builder.set(CaptureRequest.CONTROL_AF_MODE, 4);
			} else if (contains((int[]) this.mCharacteristics.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES),
					3)) {
				builder.set(CaptureRequest.CONTROL_AF_MODE, 3);
			} else {
				builder.set(CaptureRequest.CONTROL_AF_MODE, 1);
			}
		} else if (this.mCurrentFocusMode == 3) {
			builder.set(CaptureRequest.CONTROL_AF_MODE, 0);
			if (contains((int[]) this.mCharacteristics.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES), 1)) {
				builder.set(CaptureRequest.LENS_FOCUS_DISTANCE, 0.0f);
			}
		} else if (this.mCurrentFocusMode == 4) {
			builder.set(CaptureRequest.CONTROL_AF_MODE, 2);
			if (!repeating) {
				builder.set(CaptureRequest.CONTROL_AF_TRIGGER, 1);
			}
		} else {
			if (this.mCurrentFocusMode != 5) {
				return false;
			}
			builder.set(CaptureRequest.CONTROL_AF_MODE, 0);
			if (contains((int[]) this.mCharacteristics.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES), 1)) {
				builder.set(CaptureRequest.LENS_FOCUS_DISTANCE, 0.6f);
			}
		}
		try {
			final CameraCaptureSession.CaptureCallback callback = new CameraCaptureSession.CaptureCallback() {
				public void onCaptureStarted(final CameraCaptureSession session, final CaptureRequest request,
						final long timestamp, final long frameNumber) {
					if (!CppCameraDeviceAPI2.this.mIsTimeStampSourceRealTime
							&& CppCameraDeviceAPI2.this.mTimeStampOffset == 0L
							&& !CppCameraDeviceAPI2.this.mIsTimeStampSystemRealTime) {
						CppCameraDeviceAPI2.this.mTimeStampOffset = SystemClock.elapsedRealtimeNanos() - timestamp;
						if (Math.abs(CppCameraDeviceAPI2.this.mTimeStampOffset) < 1000000000L) {
							CppCameraDeviceAPI2.this.mTimeStampOffset = 0L;
							CppCameraDeviceAPI2.this.mIsTimeStampSystemRealTime = true;
						}
					}
				}

				public void onCaptureProgressed(final CameraCaptureSession session, final CaptureRequest request,
						final CaptureResult partialResult) {
				}

				public void onCaptureCompleted(final CameraCaptureSession session, final CaptureRequest request,
						final TotalCaptureResult result) {
					long SKEW = 0L;
					if (result.get(CaptureResult.SENSOR_ROLLING_SHUTTER_SKEW) != null) {
						SKEW = (long) result.get(CaptureResult.SENSOR_ROLLING_SHUTTER_SKEW);
					}
					long EXPOSURE_TIME = 0L;
					if (result.get(CaptureResult.SENSOR_EXPOSURE_TIME) != null) {
						EXPOSURE_TIME = (long) result.get(CaptureResult.SENSOR_EXPOSURE_TIME);
					}
					CppCameraDeviceAPI2.this.mL1 = (SKEW + EXPOSURE_TIME) / 2L;
				}
			};
			if (repeating) {
				this.mCaptureSession.setRepeatingRequest(builder.build(), callback, this.mBackgroundHandler);
			} else {
				this.mCaptureSession.capture(builder.build(), callback, this.mBackgroundHandler);
			}
		} catch (CameraAccessException e3) {
			Log.e("CppCamera", "Camera2: \n" + Log.getStackTraceString((Throwable) e3));
			return false;
		} catch (Exception e4) {
			Log.e("CppCamera", "Camera2: \n" + Log.getStackTraceString((Throwable) e4));
			return false;
		}
		return true;
	}

	private static float getRatioError(final float x, final float x0) {
		final float a = x / x0 - 1.0f;
		final float b = x0 / x - 1.0f;
		return a * a + b * b;
	}

	private Size getOptimalPreviewSize(final int width, final int height) {
		if (this.mSupportedSizes == null) {
			return null;
		}
		Size s = null;
		float minError = Float.MAX_VALUE;
		for (final Size size : this.mSupportedSizes) {
			final float error = getRatioError((float) width, (float) size.getWidth())
					+ getRatioError((float) height, (float) size.getHeight());
			if (error < minError) {
				minError = error;
				s = size;
			}
		}
		return s;
	}

	private static boolean contains(final int[] values, final int value) {
		for (final int v : values) {
			if (v == value) {
				return true;
			}
		}
		return false;
	}

	private static native void nativeCamera2Frame(final long p0, final ByteBuffer p1, final ByteBuffer p2, final ByteBuffer p3,
			final int p4, final int p5, final int p6, final int p7, final int p8, final int p9, final int p10);

	private static class AutoResetEvent {
		private Lock lock;
		private Condition condition;
		private boolean signaled;

		public AutoResetEvent() {
			this.lock = new ReentrantLock();
			this.condition = this.lock.newCondition();
			this.signaled = false;
		}

		public void set() {
			this.lock.lock();
			try {
				this.signaled = true;
				this.condition.signal();
			} finally {
				this.lock.unlock();
			}
		}

		public void waitOne() {
			while (true) {
				this.lock.lock();
				try {
					if (this.signaled) {
						this.signaled = false;
						return;
					}
					this.condition.await();
				} catch (InterruptedException ex) {
				} finally {
					this.lock.unlock();
				}
			}
		}
	}
}