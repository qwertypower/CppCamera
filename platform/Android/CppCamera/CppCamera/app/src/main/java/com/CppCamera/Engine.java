
package com.CppCamera;

import android.app.Activity;

public class Engine {
	static Activity mActivity;
	static boolean initialized = false;

	static native void cppinitialize(Object ctx);

	public static void initialize(Activity activity) {
		if (!initialized) {
			mActivity = activity;
			cppinitialize(mActivity);
			initialized = true;
		}
	}
	public static void deinitialize()
	{
		initialized = false;
	}
}
