package com.ww.playtest;

import android.graphics.SurfaceTexture;
import android.os.Bundle;
import androidx.appcompat.app.AppCompatActivity;
import android.util.Log;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;

import tv.danmaku.ijk.media.player.IMediaPlayer;
import tv.danmaku.ijk.media.player.IjkMediaPlayer;

public class IJKPlayerTestActivity extends AppCompatActivity {

    private static final String TAG = "bz_IJKPlayerTest";
    private TextureView texture_view;
    private Surface surface;
    private IjkMediaPlayer ijkMediaPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_ijkplayer_test);
        texture_view = findViewById(R.id.texture_view);
        texture_view.setSurfaceTextureListener(new TextureView.SurfaceTextureListener() {
            @Override
            public void onSurfaceTextureAvailable(SurfaceTexture surfaceTexture, int width, int height) {
                IJKPlayerTestActivity.this.surface = new Surface(surfaceTexture);
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
        if (null == ijkMediaPlayer) {
            ijkMediaPlayer = new IjkMediaPlayer();
        } else {
            ijkMediaPlayer.reset();
        }
        try {
            ijkMediaPlayer.setSurface(surface);
//            ijkMediaPlayer.setDataSource("/sdcard/aaa/vivo720p.mp4");
            //http://liveng.alicdn.com/mediaplatform/fff3c63c-9618-4222-8bc4-05abff11a43b.flv?auth_key=1632724376-0-0-528696637e4a241e94dbe5a8a75d732f
            // https://cloud.video.taobao.com/play/u/3962528240/p/1/e/6/t/1/216377890710.mp4
            // http://cloud.video.taobao.com/play/u/3962528240/p/1/e/6/t/1/216377890710.mp4

//            ijkMediaPlayer.setDataSource("http://liveng.alicdn.com/mediaplatform/fff3c63c-9618-4222-8bc4-05abff11a43b.flv?auth_key=1632724376-0-0-528696637e4a241e94dbe5a8a75d732f");
            ijkMediaPlayer.setDataSource("https://cloud.video.taobao.com/play/u/3962528240/p/1/e/6/t/1/216377890710.mp4");
//            ijkMediaPlayer.setDataSource("http://cloud.video.taobao.com/play/u/3962528240/p/1/e/6/t/1/216377890710.mp4");

            ijkMediaPlayer.setOnPreparedListener(new IMediaPlayer.OnPreparedListener() {
                @Override
                public void onPrepared(IMediaPlayer iMediaPlayer) {
                    iMediaPlayer.start();
                }
            });
            ijkMediaPlayer.prepareAsync();
        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    @Override
    protected void onPause() {
        super.onPause();
        if (null != ijkMediaPlayer) {
            ijkMediaPlayer.pause();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (null != ijkMediaPlayer) {
            ijkMediaPlayer.stop();
            ijkMediaPlayer.release();
        }
    }
}
