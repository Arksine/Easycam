package com.arksine.easycam;

import android.app.Activity;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.util.Log;

import java.io.File;
import java.util.ArrayList;

/**
 * TODO:  Create a fragment to Add and Edit entries in the devices.json file.
 * PrefenceFragment isn't robust enough to easily handle all of the functionality
 * required.  In the meantime, the devices.json file can be edited directly
 * if we need to change settings or create a new device profile.  This will probably
 * require that I implement a custom "ListPreference" for the "select device" preference.
 */

public class SettingsActivity extends Activity {

    private static String TAG = "SettingsActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getFragmentManager().beginTransaction()
                .add(android.R.id.content, new SettingsFragment())
                .commit();

    }

    public static class SettingsFragment extends PreferenceFragment {

	    private class DeviceEntry {
		    public String deviceName;
		    public String deviceLocation;
	    }

        @Override
        public void onCreate(Bundle savedInstanceState) {
	        super.onCreate(savedInstanceState);

	        // Load the preferences from an XML resource
	        addPreferencesFromResource(R.xml.preferences);

	        PreferenceScreen root = this.getPreferenceScreen();
	        ListPreference selectDevice = (ListPreference) root.findPreference("pref_key_select_device");
	        ListPreference selectStandard = (ListPreference) root.findPreference("pref_key_select_standard");
	        ListPreference selectDeint = (ListPreference) root.findPreference("pref_key_deinterlace_method");

	        populateListPreference(root);

	        /**
	         * Below are listeners for each of our device settings that update the summary based on the
	         * currently selected value.
	         */
	        selectDevice.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
		        @Override
		        public boolean onPreferenceChange(Preference preference, Object newValue) {

			        ListPreference list = (ListPreference)preference;
			        CharSequence[] entries = list.getEntries();
			        int index = list.findIndexOfValue((String)newValue);
			        preference.setSummary(entries[index]);
			        return true;
		        }
	        });

	        selectStandard.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
		        @Override
		        public boolean onPreferenceChange(Preference preference, Object newValue) {

			        ListPreference list = (ListPreference)preference;
			        CharSequence[] entries = list.getEntries();
			        int index = list.findIndexOfValue((String)newValue);
			        preference.setSummary(entries[index]);
			        return true;
		        }
	        });

	        selectDeint.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
		        @Override
		        public boolean onPreferenceChange(Preference preference, Object newValue) {

			        ListPreference list = (ListPreference)preference;
			        CharSequence[] entries = list.getEntries();
			        int index = list.findIndexOfValue((String)newValue);
			        preference.setSummary(entries[index]);
			        return true;
		        }
	        });



        }

	    private void populateListPreference(PreferenceScreen root) {


		    ListPreference list = (ListPreference) root.findPreference("pref_key_select_device");

		    ArrayList<DeviceEntry> deviceList = enumerateDevices();

		    if (deviceList.isEmpty()) {
			    Log.i(TAG, "No V4L2 compatible streaming devices found on system");
			    //TODO:  Device list is empty,  Populate List with a dummy entry

		    }
		    else {
			    CharSequence[] entries = new CharSequence[deviceList.size()];
			    CharSequence[] entryValues = new CharSequence[deviceList.size()];

			    for (int i = 0; i < deviceList.size(); i++) {

				    entries[i] = deviceList.get(i).deviceName + " @ " +
						    deviceList.get(i).deviceLocation;
				    entryValues[i] = deviceList.get(i).deviceName + ":" +
						    deviceList.get(i).deviceLocation;

			    }


			    list.setEntries(entries);
			    list.setEntryValues(entryValues);
		    }
	    }

	    /**
	     * enumerateDevices - Checks the system for V4L2 compatible video streaming devices, then
	     *                    checks to see if that device has stored default settings in the
	     *                    devices.json file.
	     *
	     * @return An array list containing valid devices found on the system
	     */
	    private ArrayList<DeviceEntry> enumerateDevices () {
		    String devLocation;
		    String driver;      // The driver name returned from V4L2
		    DeviceEntry device;
		    ArrayList<DeviceEntry> deviceList = new ArrayList<DeviceEntry>(5);

		    synchronized (JsonManager.lock) {

			    //Make sure the JsonManger has been initialized
			    if (!JsonManager.isInitialized())
				    JsonManager.initialize();

			    for (int i = 0; i < 99; i++) {
				    devLocation = "/dev/video" + String.valueOf(i);
				    File test = new File(devLocation);

				    if (test.exists()) {


					    driver = NativeEasyCapture.findDevice(devLocation);

					    if (driver.compareTo("NO_DEVICE") == 0) {
						    // The device found at this location is not compatible with v4l2 streaming
						    Log.i(TAG, "Device at " + devLocation + " not V4L2 compatible.");

					    } else {


						    // Valid V4L2 device.  Check the devices.json file to see if it is supported.
						    // if so, add the device to the list
						    if (JsonManager.checkDevice(driver)) {
							    device = new DeviceEntry();
							    device.deviceName = driver;
							    device.deviceLocation = devLocation;
							    deviceList.add(device);
						    }
					    }
				    }
			    }
		    }

		    return deviceList;
	    }

    }
}
