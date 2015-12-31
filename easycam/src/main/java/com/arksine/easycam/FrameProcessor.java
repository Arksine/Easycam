/**
 * Created by Eric on 12/29/2015.
 *
 * Class to provide a way to choose which function we need to use for color conversion.
 * Mimics a function pointer.
 *
 */

package com.arksine.easycam;

import android.content.Context;
import android.graphics.Bitmap;
import android.support.v8.renderscript.*;
import android.util.Log;
import java.nio.ByteBuffer;

public class FrameProcessor {

    private static String TAG = "FrameProcessor";

    ColorProcessor mProcessor;

    private Bitmap mOutputFrame;

    public FrameProcessor(Context context, EasycapSettings devSets) {

        // number of bytes in a yuv buffer should be width * height * 2, as there are
        // two bytes per pixel
        int allocationLength = (devSets.frameWidth * devSets.frameHeight * 2)/4;
        Log.i(TAG, "Input buffer Length (bytes): " + (allocationLength*4));

        switch (devSets.devType.second) {
            case 0:         //UTV007, YUYV
            case 1:         //EMPIA, YUYV
                mOutputFrame = Bitmap.createBitmap(devSets.frameWidth, devSets.frameHeight,
                        Bitmap.Config.ARGB_8888);
                mProcessor = new ProcessYUYV(context, allocationLength);
                break;
            case 2:         //STK1160, UYVY
            case 3:         //SOMAGIC, UYVY
                mOutputFrame = Bitmap.createBitmap(devSets.frameWidth, devSets.frameHeight,
                        Bitmap.Config.ARGB_8888);
                mProcessor = new ProcessUYVY(context, allocationLength);
                break;
            case 4:         //UVC, RGB565
                mOutputFrame = Bitmap.createBitmap(devSets.frameWidth, devSets.frameHeight,
                        Bitmap.Config.RGB_565);
                mProcessor = new ProcessRGB();
                break;
            default:        //Generic, ARGB
                mOutputFrame = Bitmap.createBitmap(devSets.frameWidth, devSets.frameHeight,
                        Bitmap.Config.ARGB_8888);
                mProcessor = new ProcessRGB();
                break;

        }
    }

    public Bitmap processFrame(byte[] frame) {
        return mProcessor.go(frame);
    }

    private interface ColorProcessor {

        public Bitmap go(byte[] inFrame);
    }

    private class ScriptSetup {

        protected RenderScript mRS;
        protected Allocation mInAllocation;
        protected Allocation mOutAllocation;
        protected ScriptC_convert mScript;

        public ScriptSetup(Context context, int bufLength) {



            mRS = RenderScript.create(context);

            // TODO:  Need to check the input allocation.
            mInAllocation = Allocation.createSized(mRS, Element.U8_4(mRS), bufLength);
            mOutAllocation = Allocation.createFromBitmap(mRS, mOutputFrame);

            mScript = new ScriptC_convert(mRS);
            mScript.set_output(mOutAllocation);
        }

    }

    private class ProcessYUYV extends ScriptSetup implements ColorProcessor  {

        public ProcessYUYV(Context context, int bufLength) {
            super(context, bufLength);
        }

        @Override
        public Bitmap go(byte[] inFrame) {

            mInAllocation.copyFromUnchecked(inFrame);
            mScript.forEach_convertFromYUYV(mInAllocation);
            mOutAllocation.copyTo(mOutputFrame);
            return mOutputFrame;
        }
    }

    private class ProcessUYVY extends ScriptSetup implements ColorProcessor {

        public ProcessUYVY(Context context, int bufLength) {
            super(context, bufLength);
        }

        @Override
        public Bitmap go(byte[] inFrame) {
            mInAllocation.copyFromUnchecked(inFrame);
            mScript.forEach_convertFromUYVY(mInAllocation);
            mOutAllocation.copyTo(mOutputFrame);
            return mOutputFrame;
        }
    }

    private class ProcessRGB implements ColorProcessor {

        @Override
        public Bitmap go(byte[] inFrame) {
            ByteBuffer buf = ByteBuffer.wrap(inFrame);
            mOutputFrame.copyPixelsFromBuffer(buf);
            return mOutputFrame;
        }
    }

}
