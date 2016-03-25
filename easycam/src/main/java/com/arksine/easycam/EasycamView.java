

package com.arksine.easycam;


import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.graphics.Rect;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.preference.PreferenceManager;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.Toast;

/**
 *  Class: EasycamView
 *
 *  Provides a surface to draw incoming frames on
 */
public class EasycamView extends SurfaceView implements
SurfaceHolder.Callback, Runnable {

	private static String TAG = "EasycamView";
	
	private EasyCapture capDevice;
	
	private Thread mThread = null;

    private Rect mViewWindow;
    private Context appContext;
    private volatile boolean mRunning = true;
    private volatile SurfaceHolder mHolder;

    SharedPreferences sharedPrefs;

    private boolean requestUsbPermission = true;

    private static final String ACTION_USB_PERMISSION = "com.arksine.easycam.USB_PERMISSION";
    private PendingIntent mPermissionIntent;

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

                                // If the device has a valid v4l2 driver, its added to the supported
                                // device list and the Select Device ListPreference is updated
                                if (checkV4L2Device(tmpDev)) {
                                    populateDeviceListPreference();
                                }
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


    public EasycamView(Context context) {
        super(context);
        appContext = context;
        sharedPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        init();
    }

    public EasycamView(Context context, AttributeSet attrs) {
        super(context, attrs);
        appContext = context;
        sharedPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        init();
    }


    private void init() {
        Log.d(TAG, "EasycamView constructed");
        setFocusable(true);
        setBackgroundColor(0);

        mHolder = getHolder();
        mHolder.addCallback(this);

    }

    @Override
    public void run() {
        while(mRunning) {
            if (capDevice.isAttached()) {
                capDevice.getFrame(mHolder.getSurface());

            }
            else {
                mRunning = false;
            }

        }
    }

    protected Rect getViewingWindow() {
        return mViewWindow;
    }

    private void setViewingWindow(int winWidth, int winHeight) {

        mViewWindow = new Rect(0,0,winWidth,winHeight);

    }

    public void resume() {

        if (mThread != null && mThread.isAlive())
        {
            mRunning = false;
            try {
                mThread.join();
            }
            catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

        /**
         * TODO: 3/24/2016
         * I need to get the selected usb device from shared preferences here.  Then I need to use
         * check the usbpermission requested shared preference, and the UsbManager hasPermission().
         * If I need to request permission I will do so here.
         * If its granted, I need to detect the device file location (/dev/video0).  If that is found, the code
         * below is executed.
         *
         * If requestusbpermission is not set, I simply need to find the device file and execute
         * the code below.  I can also check the UsbManager hasPermission function on the usb device
         * to
         *
         *
         */


         requestUsbPermission = usbPermission.isChecked();
         Log.d(TAG, "Request Usb permission is set to " + String.valueOf(requestUsbPermission));


    }

    private void initResume() {

        capDevice = new NativeEasyCapture(sharedPrefs, appContext);
        if(!capDevice.isDeviceConnected())
        {
            CharSequence text = "Error connecting to device";
            int duration = Toast.LENGTH_SHORT;
            Toast toast = Toast.makeText(appContext, text, duration);
            toast.show();

            Log.e(TAG, "Error connecting device");
            mRunning = false;
            return;
        }
        Log.i(TAG, "View resumed");


        // Attempt to start streaming
        if (!capDevice.streamOn()) {
            CharSequence text = "Unable to stream video from device";
            int duration = Toast.LENGTH_SHORT;
            Toast toast = Toast.makeText(appContext, text, duration);
            toast.show();

            Log.e(TAG, "Device error: unable to start streaming video");
            mRunning = false;
            return;
        }

        mRunning = true;
        mThread = new Thread(this);
        mThread.start();
    }

    public void pause()  {

        mRunning = false;
        if (mThread != null) {
            boolean retry = true;
            while (retry) {
                try {
                    mThread.join();
                    retry = false;
                } catch (InterruptedException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
        }
        capDevice.stop();
        Log.i(TAG, "View paused");
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.d(TAG, "Surface created");

        // Set up the intent necessary to get request access for a USB device
        mPermissionIntent = PendingIntent.getBroadcast(getContext(), 0, new Intent(ACTION_USB_PERMISSION), 0);
        IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
        getContext().registerReceiver(mUsbReceiver, filter);

        resume();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.d(TAG, "Surface destroyed");
        getContext().unregisterReceiver(mUsbReceiver);
        pause();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int winWidth,
                               int winHeight) {
        Log.d("Easycam", "surfaceChanged");

        setViewingWindow (winWidth, winHeight);

    }

}
