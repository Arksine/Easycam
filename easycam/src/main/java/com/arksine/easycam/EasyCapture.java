package com.arksine.easycam;

import android.view.Surface;

public interface EasyCapture {
	public void getFrame(Surface mySuface);
    public void stop();
    public boolean isAttached();
    public boolean isDeviceConnected();
    public boolean streamOn();
}
