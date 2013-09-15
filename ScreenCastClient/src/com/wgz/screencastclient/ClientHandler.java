package com.wgz.screencastclient;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.Socket;

import android.os.IBinder;
import android.os.RemoteException;
//import android.os.ServiceManager;
//import android.view.IWindowManager;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.hardware.input.InputManager;
import android.util.Log;
import android.os.SystemClock;
import android.view.InputDevice;



public class ClientHandler {
//	IBinder wmbinder = ServiceManager.getService( "window" );
//	final IWindowManager wm = IWindowManager.Stub.asInterface( wmbinder );
    Socket s;

	public ClientHandler(Socket s) throws IOException, RemoteException {
		this.s = s;
		Thread tSend = new Thread() {
			public void run() {
				sendFrameBuffer();
			}
		};
		
		Thread tHandleCmd = new Thread() {
			public void run() {
				handleCmd();
			}
		};
		tSend.start();
		tHandleCmd.start();
		
		try {
			tSend.join();
			tHandleCmd.join();
		} catch (InterruptedException e) {
		}
	}
	
	private void sendFrameBuffer() {
		try {
			Process p = Runtime.getRuntime().exec("/system/bin/cat /dev/graphics/fb0");
			InputStream is = p.getInputStream();
			Log.e("screencast","Starting sending framebuffer");
			OutputStream os = s.getOutputStream();
			byte[] buff = new byte[336*512*2];
			while(true) {
				//FileInputStream fos = new FileInputStream("/dev/graphics/fb0");
				int nb = is.read(buff);
				if(nb < -1)
					break;
				//fos.close();
				Log.e("screencast","val "+nb);
				os.write(buff,0,nb);
				Thread.sleep(10);
			}
			is.close();
			System.out.println("End of sending thread");
			
		} catch(Exception ex) {
			ex.printStackTrace();
		}
	}
	
	private void handleCmd() {
		try {
			InputStream is = s.getInputStream();
			BufferedReader r = new BufferedReader(new InputStreamReader(is));
			while(true) {
	    		String line = r.readLine();
	    		if(line == null) {
	    			r.close();
	    			s.close();
	    			break;
	    		}
	    	
	    		Log.e("screencast","Received : "+line);
	    		try {
	    			handleCommand(line);
	    		} catch(Exception ex) {
	    			ex.printStackTrace();
	    		}
			}
		} catch(Exception ex) {
			ex.printStackTrace();
		}
	}
	
	private void handleCommand(String line) throws RemoteException {
		String[] paramList = line.split("/");
		String type = paramList[0];
		if(type.equals("quit")) {
			System.exit(0);
			return;
		}
		if(type.equals("pointer")) {
			Log.e("screencast","sendpoint");
			InputManager.getInstance().injectInputEvent(getMotionEvent(paramList), InputManager.INJECT_INPUT_EVENT_MODE_WAIT_FOR_FINISH);
			return;
		}
		if(type.equals("key")) {
			Log.e("screencast","key");
			InputManager.getInstance().injectInputEvent(getKeyEvent(paramList), InputManager.INJECT_INPUT_EVENT_MODE_WAIT_FOR_FINISH);
			return;
		}
		if(type.equals("trackball")) {
			Log.e("screencast","trackball");
			InputManager.getInstance().injectInputEvent(getMotionEvent(paramList), InputManager.INJECT_INPUT_EVENT_MODE_WAIT_FOR_FINISH);
			return;
		}
		
		throw new RuntimeException("Invalid type : "+type);

	}
	
    private static MotionEvent getMotionEvent(String[] args) {
    	int i = 1;
		long now = SystemClock.uptimeMillis();
    	long downTime = now;i++;
    	long eventTime = now;i++;
    	int action = Integer.parseInt(args[i++]);
    	float x = Float.parseFloat(args[i++]);
    	float y = Float.parseFloat(args[i++]);
    	int metaState = Integer.parseInt(args[i++]);
        MotionEvent event = MotionEvent.obtain(downTime, eventTime, action, x, y, metaState);
		event.setSource(InputDevice.SOURCE_TOUCHSCREEN);
		return event;
    }

    private static KeyEvent getKeyEvent(String[] args) {
		long now = SystemClock.uptimeMillis();
    	int action = Integer.parseInt(args[1]);
    	int code = Integer.parseInt(args[2]);
    	return new KeyEvent(now , now ,action, code , 0);
    }
}
