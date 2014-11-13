package com.arksine.easycamlib;

import java.io.File;

import android.content.SharedPreferences;
import android.util.Log;
import android.util.Pair;


public class EasycapSettings {
	
	private static String TAG = "EasyCapSettings";
	
	public final Pair<String, Integer> devType;
	public final Pair<String, Integer> devStandard;
	public final String devName;
	public final int frameWidth;
	public final int frameHeight;
	public final int numBuffers;
	
	
	EasycapSettings(SharedPreferences sharedPrefs) {
		
		boolean prefSetDevManual = sharedPrefs.getBoolean("pref_key_manual_set_dev_loc", false);
		if (prefSetDevManual)
		{
			String prefDevName = sharedPrefs.getString("pref_select_dev_loc", "/dev/video0");
			devName = prefDevName;
		}
		else
		{
			// autoselect device location
			devName = checkDevices();
		}
		boolean prefSetDevTypeManual = sharedPrefs.getBoolean("pref_key_manual_set_type", false);
		if (prefSetDevTypeManual) {
			String prefDevice = sharedPrefs.getString("pref_select_easycap_type", "0");
			switch(Integer.valueOf(prefDevice)) {
			case 0:
				devType = Pair.create("UTV007", 0);
				numBuffers = 2;
				break;
			case 1:
				devType = Pair.create("EMPIA", 1);
				numBuffers = 2;
				break;
			case 2:
				devType = Pair.create("STK1160", 2);
				numBuffers = 2;
				break;
			case 3:
				devType = Pair.create("SOMAGIC", 3);
				numBuffers = 4;
				break;		
			default: 
				devType = Pair.create("Default", 4);
				numBuffers = 2;							
			}
		}
		else {
			String prefDevice = NativeEasycam.autoDetectDev(devName);
						
			if(prefDevice.equals("UTV007")) {
				devType = Pair.create("UTV007", 0);
				numBuffers = 2;				
			}				
			else if(prefDevice.equals("EMPIA")) {
				devType = Pair.create("EMPIA", 1);
				numBuffers = 2;	
			}
			else if(prefDevice.equals("STK1160")) {
				devType = Pair.create("STK1160", 2);
				numBuffers = 2;	
			}
			else if(prefDevice.equals("SOMAGIC")) {
				devType = Pair.create("SOMAGIC", 3);
				numBuffers = 4;	
			}
			else {
				devType = Pair.create("Default", 4);
				numBuffers = 2;	
			}
		}
		
		String prefStandard = sharedPrefs.getString("pref_select_standard", "0");
		switch(Integer.valueOf(prefStandard)) {
		case 0:
			devStandard = Pair.create("NTSC", 0);

            // Empia devices like a frame width of 640 pixels
            if (devType.second == 1)
                frameWidth = 640;
            else
			    frameWidth = 720;
			
			// Somagic devices have 484 NTSC lines
			if (devType.second == 3)
				frameHeight = 484;
			else
				frameHeight = 480;
			break;
		case 1:
			devStandard = Pair.create("PAL", 1);
			frameWidth = 720;
			frameHeight = 576;
			break;
			
			default: 
				devStandard = Pair.create("NTSC", 0);
				frameWidth = 720;
				frameHeight = 480;
				break;				
		}

		//Logging for debugging
		Log.i(TAG, "Currently set device file: " + devName);
		Log.i(TAG, "Currently set device type: " + devType.first);
		Log.i(TAG, "Currently set tv standard: " + devStandard.first);
		Log.i(TAG, "Currently set frame width: " + frameWidth);
		Log.i(TAG, "Currently set frame height: " + frameHeight);
	}	
	
    // iterate through the video devices and choose the first one
    private String checkDevices () {	
		String fName;
	
		for (int i=0; i<10; i++)
		{
			fName = "/dev/video" + String.valueOf(i);
			File test = new File(fName);
			
			if (test.exists()){
				return fName;
			 			
			}    			
		}
		
		fName = "/dev/video0";
		return fName;
	}
    
}
