package com.arksine.easycam;

import android.app.Activity;
import android.content.Context;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.util.Log;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;

/**
 * TODO: 3/24/2016
 * Need to move requestpermission to the easycamview class.  Otherwise the application will
 * attempt to acesss the device without permission when its launched without entering settings
 */

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
			public String deviceDescription;
		    public String deviceName;

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

			ArrayList<DeviceEntry> validStreamingDeviceList = enumerateUsbDevices();
			populateDeviceListPreference(validStreamingDeviceList, selectDevice);

			// Set the summaries for preferences that assign them dynamically
			selectDevice.setSummary(selectDevice.getEntry());
			selectStandard.setSummary(selectStandard.getEntry());
			selectDeint.setSummary(selectDeint.getEntry());

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

	    private void populateDeviceListPreference(ArrayList<DeviceEntry> validStreamingDeviceList,
												  ListPreference selectDevice) {

			CharSequence[] entries;
			CharSequence[] entryValues;

		    if (validStreamingDeviceList.isEmpty()) {

			    Log.i(TAG, "No V4L2 compatible streaming devices found on system");
				entries = new CharSequence[1];
				entryValues = new CharSequence[1];

				entries[0]  = "No devices found";
				entryValues[0] = "NO_DEVICE";

				selectDevice.setSummary(entries[0]);

		    }
		    else {

			    entries = new CharSequence[validStreamingDeviceList.size()];
			    entryValues = new CharSequence[validStreamingDeviceList.size()];

			    for (int i = 0; i < validStreamingDeviceList.size(); i++) {

				    entries[i] = validStreamingDeviceList.get(i).deviceDescription;
				    entryValues[i] = validStreamingDeviceList.get(i).deviceName;

			    }
		    }

			selectDevice.setEntries(entries);
			selectDevice.setEntryValues(entryValues);
	    }

	    /**
	     * checkV4L2Device - Checks the system for V4L2 compatible video streaming devices, then
	     *                    checks to see if that device has stored default settings in the
	     *                    devices.json file.
	     *
	     */

		private ArrayList<DeviceEntry> enumerateUsbDevices() {

			ArrayList<DeviceEntry> validStreamingDeviceList = new ArrayList<>(5);

			synchronized (JsonManager.lock) {

				//Make sure the JsonManger has been initialized
				if (!JsonManager.isInitialized())
					JsonManager.initialize();


				// Enumerate a list of currently connected Usb devices so we can pick out which
				// ones are supported easycap devices
				UsbManager mUsbManager = (UsbManager) getActivity().getSystemService(Context.USB_SERVICE);
				HashMap<String, UsbDevice> usbDeviceList = mUsbManager.getDeviceList();
				Iterator<UsbDevice> deviceIterator = usbDeviceList.values().iterator();

				// Make sure the device list is empty before enumeration
				validStreamingDeviceList.clear();

				while(deviceIterator.hasNext()) {

					UsbDevice uDevice = deviceIterator.next();

					DeviceInfo devInfo = JsonManager.getDevice(uDevice.getVendorId(), uDevice.getProductId(),
							DeviceInfo.DeviceStandard.NTSC);

					// If a supported device is listed in json list, request permission
					// to access it
					if (devInfo != null) {

						Log.i(TAG, "Supported usb device found: " + uDevice.toString());
						Log.i(TAG, "Device ID: " + uDevice.getDeviceId());
						Log.i(TAG, "Device Name: " + uDevice.getDeviceName());
						Log.i(TAG, "Vendor: ID " + uDevice.getVendorId());
						Log.i(TAG, "Product ID: " + uDevice.getProductId());

						DeviceEntry devEntry = new DeviceEntry();

						// TODO: Rather than use the driver as a discriptor, I should add a field
						// to the devices.json file with the devices lsusb name (ie UTV007 for usbtv)
						devEntry.deviceDescription = devInfo.getDriver() + " @ " + uDevice.getDeviceName();
						devEntry.deviceName = uDevice.getDeviceName() + ":" + devInfo.getDriver();

						validStreamingDeviceList.add(devEntry);

					}
				}

			}

			return validStreamingDeviceList;
		}
    }
}
