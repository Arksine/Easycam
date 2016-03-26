

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

import java.util.HashMap;

/**
 *  Class: EasycamView
 *
 *  Provides a surface to draw incoming frames on
 */
public class EasycamView extends SurfaceView implements
SurfaceHolder.Callback, Runnable {

	private static String TAG = "EasycamView";
	
	private EasyCapture capDevice = null;
	
	private Thread mThread = null;

    private Rect mViewWindow;
    private Context appContext;
    private volatile boolean mRunning = true;
    private volatile SurfaceHolder mHolder;

    SharedPreferences sharedPrefs;

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

                           initThread();
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

        boolean requestUsbPermission = sharedPrefs.getBoolean("pref_key_request_usb_permission", true);
        Log.d(TAG, "Request Usb permission is set to " + String.valueOf(requestUsbPermission));

        String prefSelectDevice = sharedPrefs.getString("pref_key_select_device", "NO_DEVICE");
        if (prefSelectDevice.equals("NO_DEVICE")){
            Log.e(TAG, "No device selected in preferences");
            // No device selected, exit.
            return;
        }

        String[] devDesc = prefSelectDevice.split(":");

        UsbManager mUsbManager = (UsbManager) getContext().getSystemService(Context.USB_SERVICE);
        HashMap<String, UsbDevice> usbDeviceList = mUsbManager.getDeviceList();
        UsbDevice uDevice = usbDeviceList.get(devDesc[0]);

        // If the request usb permission is set in user settings and the device does not have
        // permission then we will request permission
        if(requestUsbPermission && !(mUsbManager.hasPermission(uDevice))) {
            mUsbManager.requestPermission(uDevice, mPermissionIntent);
        }
        else {
            initThread();
        }

    }

    private void initThread() {

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

        if (capDevice != null)
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
