package com.arksine.easycamlib;

import android.graphics.Bitmap;

public interface Easycam {
	public Bitmap getFrame();
    public void stop();
    public boolean isAttached();
    public boolean isDeviceConnected();
}
