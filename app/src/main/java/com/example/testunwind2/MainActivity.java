package com.example.testunwind2;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity implements View.OnClickListener{

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    public Handler mHandler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        HandlerThread handlerThread = new HandlerThread("test_unwind");
        handlerThread.start();
        mHandler = new Handler(handlerThread.getLooper());

//        mHandler.post(new Runnable() {
//            @Override
//            public void run() {
//                installSingal();
//            }
//        });

        installSingal();

    }


    public native void installSingal();

    public native void go2Crash();

    public native void go2CrashCoffeeCatch();

    @Override
    public void onClick(View view) {
        int id = view.getId();
        if(id == R.id.btn_go2crash_natvie_currentthread){

            Log.d("feifei","current thread crash");
            go2Crash();
            //信号量安装 与crash 在同一个线程,调用unwind 可以正确捕捉crash 并提取堆栈

        }else if(id == R.id.btn_go2crash_natvie_subthread){
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    go2Crash();
                    //信号量安装 与crash 在同一个线程,调用unwind 解析堆栈 会产生二次crash
                }
            });

        }
        else if(id == R.id.btn_coffee_catcher){
            go2CrashCoffeeCatch();
        }
    }
}
