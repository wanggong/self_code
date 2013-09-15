package com.longcheer.flip2silent;

import java.util.List;
import android.app.Activity;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.IBinder;
import android.os.SystemClock;
import android.preference.PreferenceManager;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.util.Log;

public class ScreenCastService extends Service {
    private static final String TAG = "screencast";


    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "onCreate");
        
        try {
    		Log.e(TAG, "start main22");
    		
    		Thread t = new Thread() {
            	public void run() {
            		try {
            			new Main22(1324, true).execute();
            		} catch(Exception ex) {
            			ex.printStackTrace();
            		}
            		
            	}
            };
            t.start();
    	    
		} catch(Exception ex) {
			ex.printStackTrace();
		}
    }


    @Override
    public void onDestroy() {  
        super.onDestroy();
        Log.i(TAG, "onDestroy");
    }


    @Override
    public IBinder onBind(Intent arg0) {
        // TODO Auto-generated method stub
        return null;
    }
}
