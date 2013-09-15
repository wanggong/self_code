package com.wgz.screencastclient;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.ContentResolver;
import android.media.AudioManager;
import android.os.Handler;
import android.os.Message;
import android.view.KeyEvent;
import android.widget.Toast;
import android.provider.Settings;
import android.util.Log;

public class BootCompleteReceiver extends BroadcastReceiver {  

    private static final String TAG = "screencastclient";

    @Override
    public void onReceive(Context context, Intent intent) {
        final ContentResolver cr = context.getContentResolver();
        if (intent.getAction().equals(Intent.ACTION_BOOT_COMPLETED)) 
		{
			Log.e("screencast","start services");
            context.startService(new Intent(context, ScreenCastService.class));
        }
    }
}
