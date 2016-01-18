package com.arksine.easycam;

import android.util.Log;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;

//TODO:  Need to programatically move devices.json from the assets folder
//       to a location with r/w access on file system

/**
 * JsonManager - Manages reading and writing to the devices.json file.  All members and methods
 *               are static, as it is not necessary to have multiple instances of this class, and
 *               it makes it much easier to access the methods between activities and objects
 */
public class JsonManager {

	private static String TAG = "JsonManager";

	// We need this lock to synchronize accesses to methods accessing the deviceList, as multiple
	// threads will be accessing it
	public static final Object lock = new Object();

	private static boolean initialized = false;
	private static String filePath = null;
	private static JSONArray deviceList;

	//  Private constructor to prevent instantiation
	private JsonManager() {	}

	public static void setFilePath(String filePath) {
		JsonManager.filePath = filePath;
	}

	public static boolean initialize() {

		if (filePath == null) {

			Log.e(TAG, "File path not set.");
			return false;
		}

		try {
			initialized = readFile();
		}
		catch (Exception e){
			// File not found
			Log.e(TAG, "Error reading file " + filePath);
			deviceList = null;
		}

		return initialized;
	}

	// clear the data, but keep the file path for easy initialization.
	public static void clear() {
		deviceList = null;
		initialized = false;
	}

	public static boolean isInitialized() {
		return initialized;
	}

	private static boolean readFile() throws Exception {

		String jsonFileContents;
		File f1 = new File(filePath);
		FileReader fIn;

		try {
			fIn = new FileReader(f1);
		}
		catch (Exception e) {
			Log.e(TAG, "Error opening file at " + filePath);
			return false;
		}

		BufferedReader reader = new BufferedReader(fIn);
		StringBuilder sb = new StringBuilder();
		String line = null;

		line = reader.readLine();
		while(line != null) {
			sb.append(line).append("\n");
			line = reader.readLine();
		}

		jsonFileContents = sb.toString();

		reader.close();
		fIn.close();

		try {

			deviceList = new JSONArray(jsonFileContents);

		}
		catch (Exception e) {
			Log.e(TAG, "Unable to parse JSON content.");
			return false;
		}

		return true;
	}

	/**
	 * Writes the current array of JSON Objects to a file in JSON format
	 * @throws Exception
	 */
	private static void writeFile() throws Exception {

		String jsonFileContents = deviceList.toString(4);

		File f1 = new File(filePath);
		FileWriter fWriter = new FileWriter(f1);
		BufferedWriter writer = new BufferedWriter(fWriter);

		writer.write(jsonFileContents, 0, jsonFileContents.length());
		writer.close();
		fWriter.close();
	}


	public static boolean addDevice(DeviceInfo newDevice){

		// Make sure that the device isn't already in the array
		if (checkDevice(newDevice.getDriver())) {

			Log.i(TAG, "Device already added");
			return false;
		}


		JSONObject ndev = new JSONObject();
		try {
			ndev.put("driver", newDevice.getDriver());
			ndev.put("framewidth", newDevice.getFrameWidth());
			JSONObject frameheight = new JSONObject();

			// We need to know what standard was selected when this device info was created.  We
			// used the stored variable for the standard selected, and a default value for the other
			if (newDevice.getDevStd() == DeviceInfo.DeviceStandard.NTSC) {
				frameheight.put("ntsc", newDevice.getFrameHeight());
				frameheight.put("pal", 576);  // default since NTSC is selected
			}
			else {
				frameheight.put("ntsc", 480);
				frameheight.put("pal", newDevice.getFrameHeight());  // default since PAL is selected
			}

			ndev.put("frameheight", frameheight);
			ndev.put("numbuffers", newDevice.getNumBuffers());
			ndev.put("pixelformat", newDevice.getPixFmt().toString());
			ndev.put("fieldtype", newDevice.getFieldType().toString());
			ndev.put("input", newDevice.getInput());

			// Add to array
			deviceList.put(ndev);
		}
		catch (Exception e) {
			Log.e(TAG, "Unable to add device to JSON Array,");
			return false;
		}

		// Write Changes to the JSON File
		try {
			writeFile();
		}
		catch (Exception e) {
			Log.e(TAG, "Unable to write changes to file at " + filePath);
			return false;
		}

		return true;
	}

	public static boolean editDevice(DeviceInfo currentDevice){

		try {
			JSONObject curJsonObj = findDevice(currentDevice.getDriver());
			
			// Somehow the device isn't found, so we'll add it to the array
			if (curJsonObj == null) {
				Log.i(TAG, currentDevice.getDriver() + " not found in JSON file, adding.");
				return addDevice(currentDevice);
			}
			
			// Replace the current values in the JSON Array with values from the currently
			// selected device
			curJsonObj.put("framewidth", currentDevice.getFrameWidth());
			JSONObject frameheight = new JSONObject();

			// Since we are only editing this device, there is no need to overwrite default values
			// for the standard not selected
			if (currentDevice.getDevStd() == DeviceInfo.DeviceStandard.NTSC) {
				frameheight.put("ntsc", currentDevice.getFrameHeight());
			}
			else {
				frameheight.put("ntsc", 480);
			}

			curJsonObj.put("frameheight", frameheight);
			curJsonObj.put("numbuffers", currentDevice.getNumBuffers());
			curJsonObj.put("pixelformat", currentDevice.getPixFmt().toString());
			curJsonObj.put("fieldtype", currentDevice.getFieldType().toString());
			curJsonObj.put("input", currentDevice.getInput());

		}
		catch (Exception e) {
			Log.e(TAG, "Unable to edit device in JSON Array,");
			return false;
		}

		// Write Changes to the JSON File
		try {
			writeFile();
		}
		catch (Exception e) {
			Log.e(TAG, "Unable to write changes to file at " + filePath);
			return false;
		}
		return true;

	}

	public static DeviceInfo getDevice(String drv, DeviceInfo.DeviceStandard std) {

		DeviceInfo curDevice = new DeviceInfo();

		try {
			JSONObject curJsonObj = findDevice(drv);

			if (curJsonObj == null) {
				Log.i(TAG, curDevice.getDriver() + " not found in JSON file.");
				return null;
			}

			curDevice.setDriver(curJsonObj.getString("driver"));
			curDevice.setFrameWidth(curJsonObj.getInt("framewidth"));

			JSONObject frameheight = curJsonObj.getJSONObject("frameheight");
			if (std == DeviceInfo.DeviceStandard.NTSC) {
				curDevice.setFrameHeight(frameheight.getInt("ntsc"));
				curDevice.setDevStd(DeviceInfo.DeviceStandard.NTSC);
			}
			else {
				curDevice.setFrameHeight(frameheight.getInt("pal"));
				curDevice.setDevStd(DeviceInfo.DeviceStandard.PAL);
			}

			curDevice.setNumBuffers(curJsonObj.getInt("numbuffers"));
			curDevice.setPixFmt(DeviceInfo.PixelFormat.valueOf(curJsonObj.getString("pixelformat")));
			curDevice.setFieldType(DeviceInfo.FieldType.valueOf(curJsonObj.getString("fieldtype")));
			curDevice.setInput(curJsonObj.getInt("input"));

		}
		catch (Exception e) {
			Log.e(TAG, "Unable to set device info.");
			return null;
		}
		return curDevice;
	}

	// Checks the JSON array to see if the device is available.  If so, return true
	public static boolean checkDevice(String driver) {

		JSONObject jObj = findDevice(driver);

		return (jObj == null);
	}

	private static JSONObject findDevice(String driver) {

		JSONObject curJsonObj = null;

		try {
			boolean found = false;
			int i = 0;

			while (i < deviceList.length() && !found) {
				curJsonObj = deviceList.getJSONObject(i);

				// find the device in the JSON array by comparing the driver name to the one
				// in the info received
				if (curJsonObj.getString("driver").compareTo(driver) == 0) {
					found = true;
				}
				i++;
			}

			if (!found)
				curJsonObj = null;
		}
		catch (Exception e) {
			Log.e(TAG, "Unable to find JSONObject in array.");
			curJsonObj = null;

		}
		return curJsonObj;
	}

}
