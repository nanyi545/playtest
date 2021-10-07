package com.ww.playtest;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
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

        mVideoView.setVideoPath("http://livecb.alicdn.com/mediaplatform/649a7024-a04d-4c86-901b-2af6bcf698ec.flv?auth_key=1634477805-0-0-2dc8a00523be89d3eec44e0e04b98883");
        mVideoView.start();
    }

    @Override
    protected void onDestroy() {
        mVideoView.release(true);
        super.onDestroy();
    }
}
