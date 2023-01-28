package com.example.jcmplayer;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.graphics.Color;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;

import android.opengl.GLSurfaceView;
import android.util.TypedValue;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Button;
import android.view.View;

import android.net.Uri;

import android.app.Activity;

import android.Manifest;

import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.content.pm.PackageManager;

public class MainActivity extends AppCompatActivity {

    private  final int FILE_VIDEO_REQUEST_CODE = 100;
    private  final int REQUEST_EXTERNAL_STORAGE = 1;
    private  String[] PERMISSIONS_STORAGE = {Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE };

    private GLSurfaceView m_glSurfaceView;
    private JCMPlayer     m_jcmplayer;
    private SeekBar       m_seekBar;

    private float         m_videoRatio;
    private float         m_videoTotalSeconds;

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

        verifyStoragePermissions(this);
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        m_videoRatio = 0;
        m_videoTotalSeconds =0;

        m_seekBar = (SeekBar)findViewById(R.id.seekBar);
        m_seekBar.bringToFront();
        m_seekBar.setProgress(0);
        m_seekBar.setOnSeekBarChangeListener(onSeekBarChangeListener);

        m_jcmplayer = new JCMPlayer(this);

        m_glSurfaceView = (GLSurfaceView)findViewById(R.id.surfaceView);
        m_glSurfaceView.setEGLContextClientVersion(3);
        m_glSurfaceView.setRenderer(m_jcmplayer);
        m_glSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY); // GLSurfaceView.RENDERMODE_WHEN_DIRTY



        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setDisplayOptions(ActionBar.DISPLAY_SHOW_CUSTOM);
            actionBar.setCustomView(R.layout.actionbar_layout);

            TextView barText=(TextView)actionBar.getCustomView().findViewById(R.id.actionbar_textView);
            Button barBtn=(Button)actionBar.getCustomView().findViewById(R.id.actionbar_button);

            barBtn.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
                    intent.setType("video/*");
                    intent.addCategory(Intent.CATEGORY_OPENABLE);
                    try {
                        startActivityForResult( Intent.createChooser(intent, "Select a File to Upload"), FILE_VIDEO_REQUEST_CODE);
                    } catch (android.content.ActivityNotFoundException ex) {
                        Log.d("FileDialog","Open File Dialog Error...");
                    }
                }
            });
        }

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);   //应用运行时，保持屏幕高亮，不锁屏
    }

    @Override
    protected void onStart()
    {
        super.onStart();
        m_jcmplayer.InitVideoPlayer();
    }


    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        m_jcmplayer.StopVideoPlayer();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data)  {
        switch (requestCode) {
            case FILE_VIDEO_REQUEST_CODE:
                if (resultCode == RESULT_OK) {
                    Uri uri = data.getData();
                    String path = FileUtils.getFilePathFromUri(this,uri);
                    Log.d("MainActivity","File Path: "+path);
                    m_jcmplayer.StartVideoPlayerWithPath(path);
                    m_videoRatio = m_jcmplayer.GetVideoSizeRatio();
                    m_videoTotalSeconds = m_jcmplayer.GetVideoTotalSeconds();
                    Log.d("MainActivity","video ratio: "+m_videoRatio);
                    Log.d("MainActivity","video total seconds: "+ m_videoTotalSeconds);
                    m_seekBar.setProgress(0);
                }
                break;
        }
        super.onActivityResult(requestCode, resultCode, data);

        updateVideoSize();
    }

    public void UpdateVideoRenderCallback()
    {
        m_glSurfaceView.requestRender();

    }

    private void updateVideoSize()
    {

        WindowManager manager = this.getWindowManager();
        DisplayMetrics outMetrics = new DisplayMetrics();
        manager.getDefaultDisplay().getMetrics(outMetrics);
        int width = outMetrics.widthPixels;
        int height = outMetrics.heightPixels;


        int topMargin = 0;
        if(m_videoRatio > 1){
            topMargin = 200;
        }

        ConstraintLayout.LayoutParams layoutParams=(ConstraintLayout.LayoutParams)m_glSurfaceView.getLayoutParams();
        layoutParams.topMargin = topMargin;
        layoutParams.leftMargin = 0;
        layoutParams.rightMargin = 0;
        layoutParams.width = width;
        layoutParams.height = (int)(width / m_videoRatio);

        m_glSurfaceView.setLayoutParams(layoutParams);
    }

    private SeekBar.OnSeekBarChangeListener onSeekBarChangeListener = new SeekBar.OnSeekBarChangeListener() {
        @Override
        public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
           if(progress % 10 == 0){
               Log.d("","当前进度：" + progress);

               onSeekingVideo(progress);
           }
        }

        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {
        }

        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {
            Log.d("","结束：" + seekBar.getProgress());
            int currValue = seekBar.getProgress();
            onSeekingVideo(currValue);
        }
    };

    private void onSeekingVideo(float aValue){

        float currSliderRatio = aValue /1000.0f;
        float seekingTime = currSliderRatio * m_videoTotalSeconds;

        if(seekingTime > m_videoTotalSeconds){
            seekingTime = m_videoTotalSeconds;
        }
        Log.d("MainActivity","Seeking Time: "+ seekingTime);
        m_jcmplayer.SeekVideoPlayer(seekingTime);
    }

    private  void verifyStoragePermissions(Activity activity) {

        int permission = ActivityCompat.checkSelfPermission(activity, Manifest.permission.WRITE_EXTERNAL_STORAGE);
        if (permission != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(activity, PERMISSIONS_STORAGE, REQUEST_EXTERNAL_STORAGE);
        }
    }

}