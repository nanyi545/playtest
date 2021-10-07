package com.ww.playtest;

import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;

import com.ww.playerlib.JniLib;

import java.util.ArrayList;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        getWindow().getDecorView().postDelayed(new Runnable() {
            @Override
            public void run() {
//                startActivity(new Intent(MainActivity.this, IJKPlayerTestActivity.class));

                startActivity(new Intent(MainActivity.this, PlayDemoActivity.class));
//
//                startActivity(new Intent(MainActivity.this, AndroidPlayerTestActivity.class));
//                Uri uri = Uri.parse("tvtaobaoSDK://main");
//                startActivity(new Intent(Intent.ACTION_VIEW, uri));
            }
        },3000);
    }

    @Override
    protected void onResume() {
        super.onResume();
        requestPermission();
    }


    private boolean requestPermission() {
        ArrayList<String> permissionList = new ArrayList<>();
        //内存卡权限
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN && !PermissionUtil.isPermissionGranted(this, Manifest.permission.READ_EXTERNAL_STORAGE)) {
            permissionList.add(Manifest.permission.READ_EXTERNAL_STORAGE);
        }
        if (!PermissionUtil.isPermissionGranted(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)) {
            permissionList.add(Manifest.permission.WRITE_EXTERNAL_STORAGE);
        }

        String[] permissionStrings = new String[permissionList.size()];
        permissionList.toArray(permissionStrings);

        if (permissionList.size() > 0) {
            PermissionUtil.requestPermission(this, permissionStrings, PermissionUtil.CODE_REQ_PERMISSION);
            return false;
        } else {
            Log.d("ccc", "所要的权限全都有了");
            return true;
        }
    }


}
