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

            // h265硬解
//            ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER,"mediacodec-hevc", 1);

            // h264硬解
//            ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec", 1);
//            ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-auto-rotate", 1);
//            ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-handle-resolution-change", 1);

            // opensles
//            ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "opensles", 1);

// ????
//            ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_FORMAT, "dns_cache_clear", 1);


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
//            ijkMediaPlayer.setDataSource("https://cloud.video.taobao.com/play/u/3962528240/p/1/e/6/t/1/216377890710.mp4");
//            ijkMediaPlayer.setDataSource("http://cloud.video.taobao.com/play/u/3962528240/p/1/e/6/t/1/216377890710.mp4");

//            ijkMediaPlayer.setDataSource("http://liveng.alicdn.com/mediaplatform/001526a1-f171-411a-9737-f136b30f9f51.flv?auth_key=1632660262-0-0-91cba06978ad862a05beb5c2d5239171");
//            ijkMediaPlayer.setDataSource("http://liveca.alicdn.com/mediaplatform/9ada5c85-759b-46f8-8580-b8fc36f03533.flv?auth_key=1632664843-0-0-8ae974bce705e3fc795d68b8f0b5fa64");

            // h265
//            ijkMediaPlayer.setDataSource("http://liveca.alicdn.com/mediaplatform/5ccba454-0e07-4453-ac4b-fb1fc5abefd7.flv?auth_key=1632724868-0-0-d49d34361f839ed195c87a52723105e2");



            // h265
            ijkMediaPlayer.setDataSource("http://livecb.alicdn.com/mediaplatform/b780dd3d-0efd-4c37-8be2-b1ebe864006b.flv?auth_key=1633059692-0-0-d06f241964c7e4cfcb17f81671b024df");

            // h264 ...
//            ijkMediaPlayer.setDataSource("http://livecb.alicdn.com/mediaplatform/c170dd0d-2aee-4aa9-b625-8aa2175b843e.flv?auth_key=1633347411-0-0-1298c3e071b5308268319b385b2a5e37");


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
