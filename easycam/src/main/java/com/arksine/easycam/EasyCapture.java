package com.arksine.easycam;

public interface EasyCapture {
	public void getFrame();
    public void stop();
    public boolean isAttached();
    public boolean isDeviceConnected();
}
