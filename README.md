Easycam - Easycap viewer for android
=======

Easycam was created as an alternative to Carcam, an app used for displaying a backup camera in automotive installs. Easycam was built and tested on a Nexus 7 (2012) running Autodroid 1.2.   Much of Easycam is based on the excellent android-webcam library developed by the OpenXC project.  I'd like to thank OpenXC and Ford Motor Company for releasing such a great piece of open software.  Easycam uses Google's libyuv for color conversion.

Supported Devices
========

My goal when creating Easycam was to support all EZcap clones.   The status of each is as follows:

UTV007 -  Easycam was built and tested using a UTV007 based device.  These devices should function well.

STK1160 - As of the date of this writing I have been unable to test STK1160 based devices.  I believe that they *should* work without issue given that they work with Carcam, however without testing I can't be sure.  
          
em28xx  - I don't own an Empia based Easycap device.  I was however able to dig up an old KWorld USB2800 device. Unfortunately I was unable to get the driver to recognize that it was a USB2800 device.  This is likely a result of an old em28xx driver in Autodroid, or the old 3.1 kernel used.  Unfortunately because of this I can't confirm that Empia devices work.  It will be interesting to see if anyone releases a capture device using Empia's newer chipsets.  Supposedly they support UVC, which should eliminate driver issues.
          
Somagic - I do own an Easycap002, which is a Somagic based device with 4 composite inputs.  Somagic devices are tricky because they require firmware that you must extract from the windows driver.  See the following URL for info on how to extract the firmware:
          
          https://code.google.com/p/easycap-somagic-linux/wiki/GettingStarted#Extracting_firmware 
          
After extraction you need to get it on your Android device.  The firmware must be renamed smi2021_3X.bin, where X is the version of driver supplied with your device.  For example, if the windows driver file is named SmiUsbGrabber3F.sys, the extracted firmware will need to be named smi2021_3f.bin. After extraction it must be placed in the system/etc/firmware folder, the owner must be root, and the privileges should be set to 644.

After testing I WAS able to get the Easycap002 to run, but I cannot recommend it.  There are major stability issues when attempting to capture using these devices.  Dropped frames, force quits, hard resets...you name it, it happened.  Support is in the app to try them, and perhaps the single composite based Somagic devices will work better.  If you decide to try them, you have been warned.
          
Running the app
========

On first run the settings activity will show.  Make absolutely sure you set your TV Standard (region) correctly here. The app will attempt to autodetect the type of device you have and the location of the device file (ie. /dev/video0). This functionality can be toggled in the settings where you can manually enter these options, but it is not recommended to do so unless you have multiple Easycap devices hooked up to your tablet.  There are also various visual settings included, which are self explanatory.

Unlike Carcam, there is no background service that attempts to detect a signal and launch the app.  That functionality never worked well for me, as it seemed to cause stability issues rather than function as it should.  In my opinion a better way to get this functionality would be to use an Arduino to detect voltage on the reverse wire.  Obviously there would be a lot of work to get it working (step down 12v to 5v, protect from voltage spikes, an app to communicate between the Arduino and the tablet), but it
would be much more reliable than having a background loop continuously putting a drain
on your tablet's resources, having to deal with deep sleep, etc.

Building Easycam
========

Easycam was built using Android Studio 0.8.14 against Android API 19.  You will need the Android SDK and NDK. Static libraries and headers for libyuv are included in /easycam-library/src/main/thirdparty/libyuv.  As long as you set up your enviroment according to Google's guidelines there should be no issues building Easycam.

License
=======
Copyright (c) 2014 Eric Callahan licensed under the BSD license.
