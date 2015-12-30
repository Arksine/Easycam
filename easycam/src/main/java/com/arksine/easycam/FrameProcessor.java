/**
 * Created by Eric on 12/29/2015.
 *
 * Class to provide a way to choose which function we need to use for color conversion.
 * Mimics a function pointer.
 *
 */

package com.arksine.easycam;

import android.graphics.Bitmap;
import android.renderscript.*;

public class FrameProcessor {

    private Bitmap mOutputFrame;

    private RenderScript mRS;
    private Allocation mInput;
    private Allocation mOutput;
    private ScriptC_convert mScript;

    ConvertToRGB mConvertFunc;

    public FrameProcessor(EasycapSettings devSets) {

        switch (devSets.devType.second) {
            case 0:
                break;
            case 1:
                break;
            case 2:
                break;
            case 3:
                break;
            case 4:
                break;
            default:
                break;

        }
    }

    public interface ConvertToRGB {
        public void go();
    }

    public static final ConvertToRGB fromYUYV = new ConvertToRGB() {
        @Override
        public void go() {

        }
    };

    public static final ConvertToRGB fromUYVY = new ConvertToRGB() {
        @Override
        public void go() {

        }
    };

    public static final ConvertToRGB fromRGB565 = new ConvertToRGB() {
        @Override
        public void go() {

        }
    };

    public static final ConvertToRGB fromARGB = new ConvertToRGB() {
        @Override
        public void go() {

        }
    };
}
