package com.arksine.easycam;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.util.Log;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;


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

		private boolean requestUsbPermission = true;

		private static final String ACTION_USB_PERMISSION = "com.arksine.easycam.USB_PERMISSION";
		private PendingIntent mPermissionIntent;
		ArrayList<DeviceEntry> validStreamingDeviceList = new ArrayList<>(5);

	    private class DeviceEntry {
		    public String deviceName;
		    public String deviceLocation;
	    }

		private final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {

			public void onReceive(Context context, Intent intent) {
				String action = intent.getAction();
				if (ACTION_USB_PERMISSION.equals(action)) {
					synchronized (this) {
						UsbDevice uDevice = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);

						if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {

							if(uDevice != null) {

								DeviceInfo tmpDev;

								synchronized (JsonManager.lock) {
									tmpDev = JsonManager.getDevice(uDevice.getVendorId(),
									uDevice.getProductId(),
									DeviceInfo.DeviceStandard.NTSC);
								}

								if (tmpDev != null) {

									Log.d(TAG, "USB Device " + tmpDev.getDriver() + " on " +
											tmpDev.getVendorID() + ":" +
											tmpDev.getProductID() + " found");

									// We have permission to use this device, so check and see
									// if it has a v4l2 driver loaded
									checkV4L2Device(tmpDev);
								}
								else {
									Log.d(TAG, "Unable to retrive from devices.json: " + uDevice);
								}
							}
							else {
								Log.d(TAG, "USB Device not valid");
							}
						}
						else {
							Log.d(TAG, "permission denied for device " + uDevice);
						}
					}
				}
			}
		};

        @Override
        public void onCreate(Bundle savedInstanceState) {
	        super.onCreate(savedInstanceState);

			// Set up the intent necessary to get request access for a USB device
			mPermissionIntent = PendingIntent.getBroadcast(getActivity(), 0, new Intent(ACTION_USB_PERMISSION), 0);
			IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
			getActivity().registerReceiver(mUsbReceiver, filter);

	        // Load the preferences from an XML resource
	        addPreferencesFromResource(R.xml.preferences);

	        PreferenceScreen root = this.getPreferenceScreen();
	        ListPreference selectDevice = (ListPreference) root.findPreference("pref_key_select_device");
	        ListPreference selectStandard = (ListPreference) root.findPreference("pref_key_select_standard");
	        ListPreference selectDeint = (ListPreference) root.findPreference("pref_key_deinterlace_method");
			CheckBoxPreference usbPermission = (CheckBoxPreference) root.findPreference("pref_key_request_usb_permission");

			requestUsbPermission = usbPermission.isChecked();
			Log.d(TAG, "Request Usb permission is set to " + String.valueOf(requestUsbPermission));
			enumerateUsbDevices();



			// Makes sure the list preference is populated with the correct device list
			selectDevice.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {

				@Override
				public boolean onPreferenceClick(Preference preference) {

					populateDeviceListPreference((ListPreference) preference);
					return true;
				}

			});

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
			usbPermission.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
				@Override
				public boolean onPreferenceChange(Preference preference, Object newValue) {
					requestUsbPermission = (boolean)newValue;
					validStreamingDeviceList.clear();
					enumerateUsbDevices();
					return true;
				}
			});

        }

	    private void populateDeviceListPreference(ListPreference selectDevice) {

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

				    entries[i] = validStreamingDeviceList.get(i).deviceName + " @ " +
							validStreamingDeviceList.get(i).deviceLocation;
				    entryValues[i] = validStreamingDeviceList.get(i).deviceName + ":" +
							validStreamingDeviceList.get(i).deviceLocation;

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
	    private boolean checkV4L2Device (DeviceInfo dev) {

			String devLocation;
			String driver;      // The driver name returned from V4L2

			/*
			Iterate through the /dev/videoX devices located on the system
			to see if the driver for the current usb device has been loaded.
			If so, add it to the list that populates the preference fragment
			 */
			for (int i = 0; i < 99; i++) {

				devLocation = "/dev/video" + String.valueOf(i);
				File test = new File(devLocation);
				if (test.exists()) {

					// TODO: 3/24/2016
					// 		 right now the JNI function findDevice takes a location (file name) and returns
					//       the the driver name if the device is valid.  It would be better
					//		 for it to take bus info from the USB device and match it with
					//       what we have here.  Its possible that we have multiple V4l2 device
					//       that use the same driver, and the current implementation always
					//       selects the first one found.
					driver = NativeEasyCapture.findDevice(devLocation);

					if (driver.compareTo(dev.getDriver()) == 0) {
						DeviceEntry device = new DeviceEntry();
						device.deviceName = driver;
						device.deviceLocation = devLocation;
						validStreamingDeviceList.add(device);

						Log.i(TAG, "V4L2 device " + device.deviceName + " found at " +
							device.deviceLocation);
						return true;
					}
				}
			}

			return false;
	    }

		private void enumerateUsbDevices() {

			synchronized (JsonManager.lock) {

				//Make sure the JsonManger has been initialized
				if (!JsonManager.isInitialized())
					JsonManager.initialize();


				// Enumerate a list of currently connected Usb devices so we can pick out which
				// ones are supported easycap devices
				UsbManager mUsbManager = (UsbManager) getActivity().getSystemService(Context.USB_SERVICE);
				HashMap<String, UsbDevice> usbDeviceList = mUsbManager.getDeviceList();
				Iterator<UsbDevice> deviceIterator = usbDeviceList.values().iterator();
				while(deviceIterator.hasNext()){

					UsbDevice uDevice = deviceIterator.next();

					// If a supported device is listed in json list, request permission
					// to access it
					if(JsonManager.checkDevice(uDevice.getVendorId(), uDevice.getProductId())) {

						Log.i(TAG, "Supported usb device found: " + uDevice.toString());
						Log.i(TAG, "Device ID: " + uDevice.getDeviceId());
						Log.i(TAG, "Device Name: " + uDevice.getDeviceName() );
						Log.i(TAG, "Vendor: ID " + uDevice.getVendorId());
						Log.i(TAG, "Product ID: " + uDevice.getProductId());

						// If requestUsbPermission is selected in user preferences, then
						// the usbmanager will broadcast the request permission intent.  Otherwise
						// we will access the device directly.  My expectation is that with usb permission
						// I will be able to access the device on systems with SELinux enforcing.
						if (requestUsbPermission) {
							mUsbManager.requestPermission(uDevice, mPermissionIntent);
						}
						else {
							DeviceInfo tmpDev = JsonManager.getDevice(uDevice.getVendorId(),
									uDevice.getProductId(), DeviceInfo.DeviceStandard.NTSC);

							checkV4L2Device(tmpDev);
						}

					}
				}
			}
		}

		@Override
		public void onDestroy() {
			super.onDestroy();

			getActivity().unregisterReceiver(mUsbReceiver);
		}

    }
}
