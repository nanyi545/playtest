package com.ww.playerlib;

import android.util.Log;

public class JniLib {

    static {
        Log.d("TAG","JniLib-static-block");
//        System.loadLibrary("native-lib");
        System.loadLibrary("ijkplayer");
    }

}
