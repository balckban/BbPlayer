package com.example.jcmplayer;

import android.content.Context;
import android.opengl.GLSurfaceView;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import android.content.res.AssetManager;
import android.util.Log;

public class JCMPlayer implements GLSurfaceView.Renderer {

    JCMPlayer(Context ctx){ m_glContex = ctx; }

    private Context m_glContex;


    private native  void ndkInitVideoPlayer();

    private native  void ndkStartPlayerWithFile(String fileName);
    private native  void ndkPauseVideoPlay();
    private native  void ndkStopVideoPlayer();
    private native  float ndkGetVideoSizeRatio();
    private native  float ndkGetVideoTotalSeconds();


    private native  void ndkSeekMedia(float nPos);

    private native  void ndkInitGL(AssetManager assetManager);
    private native  void ndkPaintGL();
    private native  void ndkResizeGL(int width,int height);

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config){
        AssetManager assetManager = m_glContex.getAssets();
        ndkInitGL(assetManager);
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width,int height){
        ndkResizeGL(width, height);
    }

    @Override
    public void onDrawFrame(GL10 gl){
        ndkPaintGL();
    }

    public void InitVideoPlayer()
    {
        ndkInitVideoPlayer();
    }


    public void StartVideoPlayerWithPath(String fileString)
    {
        ndkStartPlayerWithFile(fileString);
    }

    public void PauseVideoPlayer()
    {
        ndkPauseVideoPlay();
    }

    public void StopVideoPlayer()
    {
        ndkStopVideoPlayer();
    }

    public float GetVideoSizeRatio()
    {
        float ratio = ndkGetVideoSizeRatio();
        return ratio;
    }

    public float GetVideoTotalSeconds()
    {
        float ratio = ndkGetVideoTotalSeconds();
        return ratio;
    }

    public void SeekVideoPlayer(float pos)
    {
        ndkSeekMedia(pos);
    }

    public void OnVideoRenderCallback()
    {
        MainActivity activity = (MainActivity)m_glContex;
        if(activity != null) {
            activity.UpdateVideoRenderCallback();
        }

    }

    static {
        System.loadLibrary("JCMPlayer");
    }


}
