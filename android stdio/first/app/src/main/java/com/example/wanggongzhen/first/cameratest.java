package com.example.wanggongzhen.first;

import android.app.Activity;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.TextureView;
import android.view.TextureView.SurfaceTextureListener;

import java.io.IOException;
import java.util.List;
import java.util.Random;


public class cameratest extends Activity implements SurfaceTextureListener {

    private TextureView mTextureView;
    private SurfaceTexture mSurfaceTexture;
    private Camera mCamera;
    private boolean isPreview = false;
    Thread mTestThread;
    Thread mTestCmdThread;
    MainHandler mHandler;
    Object takePictureWait = new Object();
    private boolean exit = false;

    MediaRecorder mMediaRecorder;

    private String TAG="cameratest";

    static final int OPENCAMERA=0;
    static final int STARTPREVIEW=1;
    static final int TAKEPICTURE=2;
    static final int TAKELONGPICTURE=3;
    static final int RELEASE=4;
    static final int EXITTHREAD=5;


    class MainHandler extends android.os.Handler
    {
        public  Object wait=new Object();
        @Override
        public void handleMessage(Message msg)
        {
            Log.d(TAG,"msg:"+msg.what);
            switch(msg.what)
            {
                case OPENCAMERA:
                    openCamera(msg.arg1);
                    break;
                case STARTPREVIEW:
                    startPreview();
                    break;
                case TAKEPICTURE:
                    takePicture();
                    break;
                case TAKELONGPICTURE:
                    longPressTakePicture(0);
                    break;
                case RELEASE:
                    relaseCamera();
                    break;
                case EXITTHREAD:
                    Looper.myLooper().quit();
                    break;

            }
            synchronized (wait)
            {
                wait.notify();
            }
        }
        public void sendMessageAndWait(int what,int arg1,int arg2)
        {
            mHandler.sendMessage(mHandler.obtainMessage(what, arg1, arg2));
            try {
                synchronized (wait)
                {
                    wait.wait();
                }
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        public void sendMessageAndWait(int what)
        {
            sendMessageAndWait(what,0,0);
        }
    }



    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mTextureView = (TextureView)findViewById(R.id.camera_preview);
        mTextureView.setSurfaceTextureListener(this);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    private void openCamera(int cameraid)
    {
        Log.d(TAG,"open camera "+cameraid);
        mCamera = Camera.open(cameraid);
        if(mCamera==null)
        {
            Log.d(TAG,"open camera error");
        }
        try {
            mCamera.setPreviewTexture(mSurfaceTexture);
            Camera.CameraInfo info = new Camera.CameraInfo();
            mCamera.getCameraInfo(cameraid,info);
            mCamera.setDisplayOrientation(info.orientation);
        } catch (IOException e) {
            e.printStackTrace();
            finish();
        }
    }

    private void startPreview()
    {
        Log.d(TAG,"start preview");
        mCamera.startPreview();
    }

    private void mSleep(int msec)
    {
        try {
            Thread.sleep(msec);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    private Camera.PictureCallback mJpegPictureCallback=new Camera.PictureCallback() {
        @Override
        public void onPictureTaken(byte[] data, Camera camera) {
            Log.d(TAG,"jpeg picture call back");
            synchronized (takePictureWait)
            {
                takePictureWait.notify();
            }
        }
    };

    private Camera.PictureCallback mRawPictureCallback=new Camera.PictureCallback() {
        @Override
        public void onPictureTaken(byte[] data, Camera camera) {
            Log.d(TAG,"raw picture call back");
        }
    };

    private Camera.ShutterCallback mShutterCallback = new Camera.ShutterCallback() {
        @Override
        public void onShutter() {
            Log.d(TAG,"shutter call back");
        }
    };

    private Camera.ShutterCallback mLongShutterCallback = new Camera.ShutterCallback() {
        @Override
        public void onShutter() {
            Log.d(TAG,"longpress shutter call back");
            mSleep(1000);
            mHandler.sendEmptyMessage(TAKELONGPICTURE);
        }
    };

    private void takePicture()
    {
        Log.d(TAG,"takepicture");
        mCamera.takePicture(mShutterCallback, mRawPictureCallback, mJpegPictureCallback);
    }

    private void longPressTakePicture(int cameraid)
    {
        Log.d(TAG, "longPressTakePicture");
        mCamera.takePicture(mLongShutterCallback, mRawPictureCallback, mJpegPictureCallback);
    }



    private void relaseCamera()
    {
        Log.d(TAG, "release camera");
        if(mCamera!=null)
        {
            if (isPreview) {
                mCamera.stopPreview();
                isPreview = false;
            }
            mCamera.release();
            mCamera =null;
        }
    }



    @Override
    public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
        mSurfaceTexture = surface;
        mTestThread = new Thread(new Runnable() {
                @Override
                public void run() {
                    Looper.prepare();
                    mHandler=new MainHandler();
                    synchronized(mTestThread) {
                        mTestThread.notify();
                    }
                    Looper.loop();
                }
        });
        mTestThread.start();
        try {
            synchronized(mTestThread) {
                mTestThread.wait();
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        mTestCmdThread = new Thread(new Runnable() {
            @Override
            public void run() {
                testCamera();
            }
        });
        mTestCmdThread.start();

    }

    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {

    }

    @Override
    protected void onDestroy()
    {
        Log.d(TAG, "enter onDestroy");
    }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
        Log.d(TAG, "enter onSurfaceTextureDestroyed");
        Log.d(TAG,"start exit mTestCmdThread");
        exit = true;
        try {
            mTestCmdThread.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        Log.d(TAG,"mTestCmdThread exited");
        mHandler.sendMessageAndWait(RELEASE);
        Log.d(TAG, "start exit mTestThread");
        mHandler.sendMessageAndWait(EXITTHREAD);
        try {
            mTestThread.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        Log.d(TAG, "mTestThread exited");
        return false;
    }

    @Override
         public void onSurfaceTextureUpdated(SurfaceTexture surface) {

    }

    private void enableShutterSound()
    {
        Random random = new Random();
        boolean enable = random.nextBoolean();
        mCamera.enableShutterSound(enable);

    }

    private void setZoom()
    {
        Camera.Parameters parameters = mCamera.getParameters();
        if(parameters.isZoomSupported())
        {
            List<Integer> ratios = parameters.getZoomRatios();
            if(ratios!=null)
            {
                int max_zoom = ratios.size();
                if(max_zoom>0)
                {
                    Random random = new Random();
                    parameters.setZoom(random.nextInt(max_zoom));
                    mCamera.setParameters(parameters);
                }
            }
        }
    }

    private void setFlahsMode()
    {
        Camera.Parameters parameters = mCamera.getParameters();
        List<String> flashMode = parameters.getSupportedFlashModes();
        if(flashMode!=null&&flashMode.size()>0)
        {
            Random random = new Random();
            String mode = flashMode.get(random.nextInt(flashMode.size()));
            Log.d(TAG, "flash mode:" + mode);
            parameters.setFlashMode(mode);
            mCamera.setParameters(parameters);
        }
    }

    private void setOtherParameters()
    {
        enableShutterSound();
        setZoom();
        setFlahsMode();
    }

    private void takePicture(int cameraid,int pic_count)
    {
        mHandler.sendMessageAndWait(OPENCAMERA, cameraid, 0);
        setOtherParameters();
        mHandler.sendMessageAndWait(STARTPREVIEW);
        mSleep(1000);
        for(int i =0;i<pic_count;i++) {
            Log.d(TAG,"count:"+i);
            mHandler.sendMessageAndWait(TAKEPICTURE);
            synchronized (takePictureWait) {
                try {
                    takePictureWait.wait();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
            setOtherParameters();
            mHandler.sendMessageAndWait(STARTPREVIEW);
            if(exit)
            {
                return;
            }
            mSleep(1000);
        }
        mHandler.sendMessageAndWait(RELEASE);
        mSleep(1000);
    }


    private void switch_test(int count)
    {
        for(int i = 0 ; i < count&&!exit;i++)
        {
            Log.d(TAG, "switch_test time:" + i);
            takePicture(0, 3);
            takePicture(1, 3);
        }
    }

    private void testCamera()
    {
        switch_test(1000000);
    }
}
