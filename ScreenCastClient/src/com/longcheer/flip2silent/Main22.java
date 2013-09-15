package com.longcheer.flip2silent;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import android.util.Log;


public class Main22 {

	static boolean debug = true;
    int port;
    
	public Main22(int port, boolean debug) {
		this.port = port;
		this.debug = debug;
	}
	
	public void execute() throws IOException 
	{
		Log.e("screencast","create socket at  ! " + port);
	    ServerSocket ss = new ServerSocket(port);
	    while(true) 
		{
            final Socket s = ss.accept();
            if(s == null || ss.isClosed())
            {
            	Log.e("screencast","socket is null or closed ! ");
            	break;
            }
            if(debug)
        	{
        		Log.e("screencast","New client ! ");
        	}
            Thread t = new Thread() 
			{
            	public void run() {
            		try {
            			new ClientHandler(s);
            		} catch(Exception ex) {
            			ex.printStackTrace();
            		}
            		try {
            			s.close();
            		} catch(Exception ex) {
            			// ignor�
            		}
            	}
            };
            t.start();
            /*
            Thread t2 = new Thread() {
            	public void run() {
            		try {
            			File f = new File("/dev/graphics/fb0");
            			FileInputStream fis = new FileInputStream(f);
            			byte[] data = new byte[16];
            			OutputStream os = s.getOutputStream();
            			while(true) {
            				int val = fis.read(data);
            				if(val <= 0) {
            					sleep(0);
            					continue;
            				}
            				os.write(data,0,val);
            				os.flush();
            			}
            		} catch(Exception ex) {
            			ex.printStackTrace();
            		}
            	}
            };
            t2.start();
            */
        }
		
	    
	}
	




    /*
    public static void keystroke2(int keycode) throws RemoteException {
    
		import android.app.Instrumentation;
		Instrumentation ins = new Instrumentation();
		ins.sendKeyDownUpSync(keycode);
    }
    */
    

}
