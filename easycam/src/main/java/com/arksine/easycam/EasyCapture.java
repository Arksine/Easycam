package com.arksine.easycam;

import android.graphics.Bitmap;

public interface EasyCapture {
	public Bitmap getFrame();
    public void stop();
    public boolean isAttached();
    public boolean isDeviceConnected();
}
