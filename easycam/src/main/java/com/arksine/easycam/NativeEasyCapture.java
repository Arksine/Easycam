package com.arksine.easycam;

import java.io.File;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.widget.Toast;

public class NativeEasyCapture implements EasyCapture {

	
    private static String TAG = "NativeEasycam";
    private EasycapSettings deviceSets;
    boolean deviceConnected = false;

    private native int startDevice(String cacheDir, String deviceName,
                                   int width, int height, int devType, int regionStd, int numBufs);
    private native void getNextFrame(Surface mySurface);
    private native boolean isDeviceAttached();
    private native void stopDevice();
    private static native String detectDevice(String deviceName);
    

    static {
        System.loadLibrary("easycapture");
    }

    public NativeEasyCapture(SharedPreferences sharedPrefs, Context context) {
    	
    	deviceSets = new EasycapSettings(sharedPrefs);
        // allocate an array of bytes to hold the entire size of the bitmap
        // at 32 bits per pixel

        boolean useToasts = sharedPrefs.getBoolean("pref_key_layout_toasts", true);
        if (useToasts) {
            // Lets show a toast telling the user what device has been set
            CharSequence text = "Device set as " + deviceSets.devType.first
                    + " at " + deviceSets.devName;
            int duration = Toast.LENGTH_SHORT;
            Toast toast = Toast.makeText(context, text, duration);
            toast.show();
        }

        connect(context);
    }

    private void connect(Context context) {
        boolean deviceReady = true;

        File deviceFile = new File(deviceSets.devName);
        if(deviceFile.exists()) {
            if(!deviceFile.canRead()) {
                Log.d(TAG, "Insufficient permissions on " + deviceSets.devName +
                        " -- does the app have the CAMERA permission?");
                deviceReady = false;
            }
        } else {
            Log.w(TAG, deviceSets.devName + " does not exist");
            deviceReady = false;
        }

        if(deviceReady) {
            Log.i(TAG, "Preparing camera with device name " + deviceSets.devName);
            if(-1 == startDevice(context.getCacheDir().toString(), deviceSets.devName,
                    deviceSets.frameWidth, deviceSets.frameHeight, deviceSets.devType.second,
                    deviceSets.devStandard.second, deviceSets.numBuffers)) {

                deviceConnected = false;
            }
            else {

                deviceConnected = true;
            }

        }
    }

    public void getFrame(Surface mySurface) {

        getNextFrame(mySurface);
    }

    public void stop() {
        stopDevice();
    }

    public boolean isAttached() {
        return isDeviceAttached();
    }

    public boolean isDeviceConnected() {return deviceConnected;}
    
    static public String autoDetectDev(String dName)
    {
    	return detectDevice(dName);
    }
}
