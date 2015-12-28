package com.arksine.easycam;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.view.Gravity;
import android.view.Menu;
import android.view.MenuItem;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.view.SurfaceHolder;
import android.view.View;
import android.content.Intent;
import android.os.Handler;
import android.widget.FrameLayout;
import android.widget.Toast;

public class easycam extends Activity {


    EasycamView camView;
    SurfaceHolder mHolder;
    FrameLayout currentLayout;

    boolean leanBackOn = true;
    boolean isFullScreen = true;
    boolean useToasts = true;

    Handler mHandler = new Handler();
    Runnable leanBackMsg = new Runnable() {
        @Override
        public void run() {
            camView.setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_FULLSCREEN);
        }
    };

    Runnable setAspectRatio = new Runnable() {
        @Override
        public void run() {
            setViewLayout();
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_easycam);

        SharedPreferences sharedPrefs =
                PreferenceManager.getDefaultSharedPreferences(this);

        // Launch a dialog on first run to select the TV standard.  Some devices
        // have issues with the incorrect standard and do not return an error
        // when doing so.
        boolean firstrun = getSharedPreferences("firstrun", MODE_PRIVATE)
                .getBoolean("pref_key_firstrun", true);
        getSharedPreferences("firstrun", MODE_PRIVATE).edit()
                .putBoolean("pref_key_firstrun", false)
                .commit();

        if (firstrun) {
            Intent intent = new Intent(this,SettingsActivity.class);
            startActivity(intent);
        }


        currentLayout = (FrameLayout) findViewById(R.id.mainLayout);
        camView = (EasycamView) findViewById(R.id.camview);
        mHolder = camView.getHolder();



        leanBackOn = sharedPrefs.getBoolean("pref_key_layout_leanback", true);
        isFullScreen = sharedPrefs.getBoolean("pref_key_layout_fullscreen", true);
        useToasts = sharedPrefs.getBoolean("pref_key_layout_toasts", true);

        camView.setOnSystemUiVisibilityChangeListener
                (new View.OnSystemUiVisibilityChangeListener() {
                    @Override
                    public void onSystemUiVisibilityChange(int visibility) {
                        if ((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0) {

                            // go into lean back mode after 3 seconds
                            if(leanBackOn) {
                                mHandler.postDelayed(leanBackMsg, 3000);
                            }
                        }
                    }
                });


        // change layout to fullscreen or 4:3 after the view has been created

    }


    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.easycam, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();
        if (id == R.id.action_settings) {

            Intent intent = new Intent(this, SettingsActivity.class);
            startActivity(intent);
            return true;
        } else if (id == R.id.toggle_fullscreen) {

            isFullScreen = !isFullScreen;
            setViewLayout();

            if(useToasts) {

                Context context = getApplicationContext();
                CharSequence text;
                int duration = Toast.LENGTH_SHORT;
                Toast toast;

                if (isFullScreen) {

                    text = "Fullscreen Mode Activated";
                    toast = Toast.makeText(context, text, duration);
                } else {

                    text = "Fullscreen Mode Deactivated";
                    toast = Toast.makeText(context, text, duration);
                }

                toast.show();
            }

            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if(leanBackOn) {
            if (hasFocus) {
                camView.setSystemUiVisibility(
                        View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                                | View.SYSTEM_UI_FLAG_FULLSCREEN);
            } else {
                // no need to have this in the queue if we lost focus
                mHandler.removeCallbacks(leanBackMsg);
            }
        }

        if (hasFocus) {
            currentLayout.post(setAspectRatio);
        }
    }

    private void setViewLayout()
    {
        FrameLayout.LayoutParams params;

        if (isFullScreen)  {

            params = new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT,
                    FrameLayout.LayoutParams.MATCH_PARENT, Gravity.CENTER);
            camView.setLayoutParams(params);


        }
        else {
            int currentWidth = currentLayout.getWidth();
            int currentHeight = currentLayout.getHeight();

            if (currentWidth >= (4 * currentHeight) / 3) {
                int destWidth = (4 * currentHeight) / 3 + 1;

                params = new FrameLayout.LayoutParams(destWidth,
                        FrameLayout.LayoutParams.MATCH_PARENT, Gravity.CENTER);
            } else {
                int destHeight = (3 * currentWidth) / 4 + 1;

                params = new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT,
                        destHeight, Gravity.CENTER);
            }

            camView.setLayoutParams(params);
        }
    }
}
