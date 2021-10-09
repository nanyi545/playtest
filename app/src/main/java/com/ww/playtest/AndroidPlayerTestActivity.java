package com.ww.playtest;

import android.graphics.SurfaceTexture;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;

import androidx.appcompat.app.AppCompatActivity;

import tv.danmaku.ijk.media.player.AndroidMediaPlayer;
import tv.danmaku.ijk.media.player.IMediaPlayer;
import tv.danmaku.ijk.media.player.IjkMediaPlayer;

public class AndroidPlayerTestActivity extends AppCompatActivity {

    private static final String TAG = "bz_IJKPlayerTest";
    private TextureView texture_view;
    private Surface surface;
    private AndroidMediaPlayer mediaPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_ijkplayer_test);
        texture_view = findViewById(R.id.texture_view);
        texture_view.setSurfaceTextureListener(new TextureView.SurfaceTextureListener() {
            @Override
            public void onSurfaceTextureAvailable(SurfaceTexture surfaceTexture, int width, int height) {
                AndroidPlayerTestActivity.this.surface = new Surface(surfaceTexture);
            }

            @Override
            public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {

            }

            @Override
            public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
                return false;
            }

            @Override
            public void onSurfaceTextureUpdated(SurfaceTexture surface) {

            }
        });
    }

    public void startPlay(View view) {
        if (null == surface) {
            Log.d(TAG, "null == surface");
            return;
        }
        if (null == mediaPlayer) {
            mediaPlayer = new AndroidMediaPlayer();



        } else {
            mediaPlayer.reset();
        }
        try {
            mediaPlayer.setSurface(surface);


            mediaPlayer.setDataSource("https://cloud.video.taobao.com/play/u/3962528240/p/1/e/6/t/1/216377890710.mp4");

            // h264 ...
//            mediaPlayer.setDataSource("http://livecb.alicdn.com/mediaplatform/c170dd0d-2aee-4aa9-b625-8aa2175b843e.flv?auth_key=1633347411-0-0-1298c3e071b5308268319b385b2a5e37");

            // h265 ...
//            mediaPlayer.setDataSource("http://livecb.alicdn.com/mediaplatform/b780dd3d-0efd-4c37-8be2-b1ebe864006b.flv?auth_key=1633059692-0-0-d06f241964c7e4cfcb17f81671b024df");

            mediaPlayer.setOnPreparedListener(new IMediaPlayer.OnPreparedListener() {
                @Override
                public void onPrepared(IMediaPlayer iMediaPlayer) {
                    iMediaPlayer.start();
                }
            });
            mediaPlayer.prepareAsync();

        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    @Override
    protected void onPause() {
        super.onPause();
        if (null != mediaPlayer) {
            mediaPlayer.pause();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (null != mediaPlayer) {
            mediaPlayer.stop();
            mediaPlayer.release();
        }
    }
}
