package com.arksine.easycam;

import java.io.File;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.Log;
import android.view.Surface;
import android.widget.Toast;

public class NativeEasyCapture implements EasyCapture {

	
    private static String TAG = "NativeEasycam";
    private DeviceInfo currentDevice;
    boolean deviceConnected = false;


    private native boolean startDevice(String cacheDir, DeviceInfo dInfo);
	private native boolean startStreaming();
    private native void getNextFrame(Surface mySurface);
    private native boolean isDeviceAttached();
    private native void stopDevice();
    private static native String detectDevice(String deviceLocation);
    

    static {
        System.loadLibrary("easycapture");
    }

    public NativeEasyCapture(SharedPreferences sharedPrefs, Context context) {

	    boolean useToasts = sharedPrefs.getBoolean("pref_key_layout_toasts", true);

	    if (getDeviceSettings(sharedPrefs)) {

		    if (useToasts) {
			    // Show a toast telling the user what device has been set
			    CharSequence text = "Device set as " + currentDevice.getDriver()
					    + " at " + currentDevice.getLocation();
			    int duration = Toast.LENGTH_SHORT;
			    Toast toast = Toast.makeText(context, text, duration);
			    toast.show();
		    }

		    connect(context);
	    } else {

		    Log.e(TAG, "Unable to load device settings.");

		    if (useToasts) {
			    CharSequence text = "Unable to load device.";
			    int duration = Toast.LENGTH_SHORT;
			    Toast toast = Toast.makeText(context, text, duration);
			    toast.show();
		    }
	    }
    }


    private void connect(Context context) {
        boolean deviceReady = true;

        File deviceFile = new File(currentDevice.getLocation());
        if(deviceFile.exists()) {
            if(!deviceFile.canRead()) {
                Log.d(TAG, "Insufficient permissions on " + currentDevice.getLocation() +
                        " -- does the app have the CAMERA permission?");
                deviceReady = false;
            }
        }
        else {
            Log.w(TAG, currentDevice.getLocation() + " does not exist");
            deviceReady = false;
        }

        if(deviceReady) {
            Log.i(TAG, "Preparing camera with device name " + currentDevice.getLocation());
            deviceConnected = startDevice(context.getCacheDir().toString(), currentDevice);
        }
    }

    private boolean getDeviceSettings (SharedPreferences sharedPrefs) {

        String prefSelectDevice = sharedPrefs.getString("pref_key_select_device", "NO_DEVICE");
        if (prefSelectDevice.compareTo("NO_DEVICE") == 0){
	        Log.e(TAG, "No device selected in preferences");
            // No device selected, exit.
            currentDevice = null;
            return false;
        }

        String prefTVStandard = sharedPrefs.getString("pref_key_select_standard", "NTSC");
        DeviceInfo.DeviceStandard std = DeviceInfo.DeviceStandard.valueOf(prefTVStandard);

        // Split the string into two parts.  The first part is the driver name, the second is the location
        String[] devAndLoc = prefSelectDevice.split(":");

        currentDevice = JsonManager.getDevice(devAndLoc[0], std);

        if (currentDevice == null) {
	        Log.e(TAG, "Unable to find device " + devAndLoc[0] + " in devices.json");
            // Device was not in devices.json, exit
            return false;
        }

        // Set the TV Standard, device location, and deinterlace method as they are not device specific and thus
        // not stored in devices.json
        currentDevice.setDevStd(std);
        currentDevice.setLocation(devAndLoc[1]);

        String deintMethod = sharedPrefs.getString("pref_key_deinterlace_method", "NONE");
	    currentDevice.setDeinterlace(DeviceInfo.DeintMethod.valueOf(deintMethod));

        //Logging for debugging
        Log.i(TAG, "Currently set device name: " + currentDevice.getDriver());
        Log.i(TAG, "Currently set device location: " + currentDevice.getLocation());
        Log.i(TAG, "Currently set tv standard: " + currentDevice.getDevStd().toString());
        Log.i(TAG, "Currently set frame width: " + currentDevice.getFrameWidth());
        Log.i(TAG, "Currently set frame height: " + currentDevice.getFrameHeight());

	    return true;
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

	public boolean streamOn() {
        return startStreaming();
	}
    
    static public String findDevice(String dName)
    {
    	return detectDevice(dName);
    }

}
