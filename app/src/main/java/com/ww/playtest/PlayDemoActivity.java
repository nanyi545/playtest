package com.ww.playtest;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.widget.TableLayout;

import com.ww.playerlib.media.AndroidMediaController;
import com.ww.playerlib.media.IjkVideoView;

public class PlayDemoActivity extends AppCompatActivity {

    private IjkVideoView mVideoView;
    TableLayout mHudView ;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_play_demo);
        mVideoView = (IjkVideoView) findViewById(R.id.video_view);
        mHudView = (TableLayout) findViewById(R.id.hud_view);
        mVideoView.setHudView(mHudView);
//        mVideoView.setVideoPath("https://cloud.video.taobao.com/play/u/3962528240/p/1/e/6/t/1/216377890710.mp4");

//         http://liveng.alicdn.com/mediaplatform/6db5410d-bebe-4ff5-8a55-611aee56394d.flv?auth_key=1635763598-0-0-fc740a98c9681d37d589530c511c2e39
        mVideoView.setVideoPath("http://liveng.alicdn.com/mediaplatform/6db5410d-bebe-4ff5-8a55-611aee56394d.flv?auth_key=1635763598-0-0-fc740a98c9681d37d589530c511c2e39");

        mVideoView.start();
        timer.sendEmptyMessage(1);
    }

    @Override
    protected void onDestroy() {
        mVideoView.release(true);
        if (null != timer) {
            timer.removeCallbacksAndMessages(null);
        }
        super.onDestroy();
    }


    Handler timer = new Handler(){
        long firstTime;
        boolean first= true;
        @Override
        public void handleMessage(@NonNull Message msg) {
            if(first){
                first =false;
                firstTime = System.currentTimeMillis();
            }
            long t = System.currentTimeMillis();
            long diff= t- firstTime;
            Log.d("ffmpeg","------------------------------main  playTime:"+(diff/1000f));
            sendEmptyMessageDelayed(1,200);
        }
    };

}
