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

	private static final String ACTION_USB_PERMISSION = "com.arksine.easycam.USB_PERMISSION";
	private PendingIntent mPermissionIntent;
	ArrayList<String> attachedUsbDeviceList = new ArrayList<>(5);

	private final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {

		public void onReceive(Context context, Intent intent) {
			String action = intent.getAction();
			if (ACTION_USB_PERMISSION.equals(action)) {
				synchronized (this) {
					UsbDevice uDevice = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);

					if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {

						if(uDevice != null) {

							DeviceInfo tmpDev = JsonManager.getDevice(uDevice.getVendorId(),
									uDevice.getProductId(),
									DeviceInfo.DeviceStandard.NTSC);

							if (tmpDev != null) {
								// We have permission to use this device, so add it to the
								// list of attached usb devices
								attachedUsbDeviceList.add(tmpDev.getDriver());
							}
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
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

		// Set up the intent necessary to get request access for a USB device
		mPermissionIntent = PendingIntent.getBroadcast(this, 0, new Intent(ACTION_USB_PERMISSION), 0);
		IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
		this.registerReceiver(mUsbReceiver, filter);

		enumerateUsbDevices();

		Bundle args = new Bundle();
		args.putStringArrayList("usblist", attachedUsbDeviceList);

		SettingsFragment setFrag = new SettingsFragment();
		setFrag.setArguments(args);
        getFragmentManager().beginTransaction()
                .add(android.R.id.content, setFrag)
                .commit();

    }

	private void enumerateUsbDevices() {

		synchronized (JsonManager.lock) {

			//Make sure the JsonManger has been initialized
			if (!JsonManager.isInitialized())
				JsonManager.initialize();

			// Enumerate a list of currently connected Usb devices so we can pick out which
			// ones are supported easycap devices
			UsbManager mUsbManager = (UsbManager) this.getSystemService(Context.USB_SERVICE);
			HashMap<String, UsbDevice> usbDeviceList = mUsbManager.getDeviceList();
			Iterator<UsbDevice> deviceIterator = usbDeviceList.values().iterator();
			while(deviceIterator.hasNext()){

				UsbDevice uDevice = deviceIterator.next();

				// If a supported device is listed in json list, request permission
				// to access it
				if(JsonManager.checkDevice(uDevice.getVendorId(), uDevice.getProductId())) {
					mUsbManager.requestPermission(uDevice, mPermissionIntent);
				}
			}
		}
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();

		this.unregisterReceiver(mUsbReceiver);
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

			ArrayList<DeviceEntry> deviceList = enumerateV4L2Devices();

		    if (deviceList.isEmpty()) {
			    Log.i(TAG, "No V4L2 compatible streaming devices found on system");

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
	     */
	    private ArrayList<DeviceEntry> enumerateV4L2Devices () {

			String devLocation;
			String driver;      // The driver name returned from V4L2
			ArrayList<DeviceEntry> deviceList = new ArrayList<>(5);
			ArrayList<String> attachedUsbDeviceList = getArguments().getStringArrayList("usblist");

			// make sure getArguments actually returned something
			if (attachedUsbDeviceList == null) {
				Log.e(TAG, "Error recieving device list");
				return deviceList;
			}

			// make sure the usb device list is actually populated
			if (attachedUsbDeviceList.isEmpty()) {
				Log.i(TAG, "No compatible usb devices found on system");
				return deviceList;
			}

			/*
			Iterate through the /dev/videoX devices located on the system
			to see if the driver for the current usb device has been loaded.
			If so, add it to the list that populates the preference fragment
			 */
			for (int i = 0; i < 99; i++) {
				devLocation = "/dev/video" + String.valueOf(i);
				File test = new File(devLocation);
				if (test.exists()) {

					driver = NativeEasyCapture.findDevice(devLocation);

					// Check to see if a valid device was found
					if (driver.compareTo("NO_DEVICE") != 0){
						/*
						Iterate through the list of attached Usb devices that we have been
						granted permission to use.  If that device driver matches the one
						return from v4l2, add it to the list of supported devices
						 */
						for (String usbDev : attachedUsbDeviceList) {
							if (driver.compareTo(usbDev) == 0) {
								DeviceEntry device = new DeviceEntry();
								device.deviceName = driver;
								device.deviceLocation = devLocation;
								deviceList.add(device);

								// Since we have added a device we can break this loop
								break;
							}
						}
					}
				}
			}


			return deviceList;

	    }

    }
}
