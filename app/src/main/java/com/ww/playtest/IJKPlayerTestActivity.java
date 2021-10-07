package com.ww.playtest;

import android.graphics.SurfaceTexture;
import android.media.AudioTrack;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.TextureView;
import android.view.View;
import android.widget.TableLayout;

import com.ww.playtest.widget.InfoHudViewHolder;

import tv.danmaku.ijk.media.player.IMediaPlayer;
import tv.danmaku.ijk.media.player.IjkMediaPlayer;

public class IJKPlayerTestActivity extends AppCompatActivity {

    private static final String TAG = "bz_IJKPlayerTest";
    private TextureView texture_view;
    private SurfaceView surfaceView;
    private Surface surface;
    private IjkMediaPlayer ijkMediaPlayer;


    TableLayout tab;
    InfoHudViewHolder info;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_ijkplayer_test);
        texture_view = findViewById(R.id.texture_view);
        tab = findViewById(R.id.hud_view);
        info = new InfoHudViewHolder(this, tab);


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
                Log.d("ffff","onSurfaceTextureUpdated ----------------------------");
            }
        });
        surfaceView = findViewById(R.id.s_view);
        surfaceView.setVisibility(View.GONE);


//        surfaceView = findViewById(R.id.s_view);
//        surfaceView.getHolder().addCallback(new SurfaceHolder.Callback2() {
//            @Override
//            public void surfaceRedrawNeeded(@NonNull SurfaceHolder holder) {
//                Log.d("ffff","SDL_Android_NativeWindow_display_l    surfaceRedrawNeeded1 -----------------------");
//
//            }
//
//            @Override
//            public void surfaceRedrawNeededAsync(@NonNull SurfaceHolder holder, @NonNull Runnable drawingFinished) {
//                Log.d("ffff","SDL_Android_NativeWindow_display_l    surfaceRedrawNeeded2 -----------------------");
//
//            }
//
//            @Override
//            public void surfaceCreated(@NonNull SurfaceHolder holder) {
//                IJKPlayerTestActivity.this.surface = holder.getSurface();
//            }
//
//            @Override
//            public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
//                Log.d("ffff","SDL_Android_NativeWindow_display_l    surfaceChanged -----------------------");
//            }
//
//            @Override
//            public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
//
//            }
//        });



    }



    public void startPlay(View view) {
        if (null == surface) {
            Log.d(TAG, "null == surface");
            return;
        }
        if (null == ijkMediaPlayer) {
            ijkMediaPlayer = new IjkMediaPlayer();

            // show info ...
//            info.setMediaPlayer(ijkMediaPlayer);


            // h265硬解
//            ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER,"mediacodec-hevc", 1);

            // h264硬解
            ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec", 1);
            ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-auto-rotate", 1);
            ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-handle-resolution-change", 1);

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
//            ijkMediaPlayer.setDataSource("http://livecb.alicdn.com/mediaplatform/8db55dc1-3900-455e-9015-713e7bee277b.flv?auth_key=1633950162-0-0-7edb36539055629fc42e137909428259");


//            ijkMediaPlayer.setDataSource("https://cloud.video.taobao.com/play/u/3962528240/p/1/e/6/t/1/216377890710.mp4");

            // http://liveng.alicdn.com/mediaplatform/bf11b745-3112-4923-97c2-bf172362d283.flv?auth_key=1634971123-0-0-45e5f4a57809314609bb3d03e833b82b
            ijkMediaPlayer.setDataSource("http://liveng.alicdn.com/mediaplatform/bf11b745-3112-4923-97c2-bf172362d283.flv?auth_key=1634971123-0-0-45e5f4a57809314609bb3d03e833b82b");


            ijkMediaPlayer.setOnPreparedListener(new IMediaPlayer.OnPreparedListener() {
                @Override
                public void onPrepared(IMediaPlayer iMediaPlayer) {
                    iMediaPlayer.start();
                    timer.sendEmptyMessage(1);
                }
            });
            ijkMediaPlayer.prepareAsync();
        } catch (Exception e) {
            e.printStackTrace();
        }

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
        if (null != timer) {
            timer.removeCallbacksAndMessages(null);
        }
    }


}
