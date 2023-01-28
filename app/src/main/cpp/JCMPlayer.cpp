//
// Created by chenchao on 2021/8/19.
//

#include "JCMPlayer.h"
#include "CCNDKCommonDef.h"
#include "JCAVCodecHandler.h"
#include "CCGLVideoRender.h"

JCAVCodecHandler    m_avCodecHandler;
CCGLVideoRender     m_glVideoRender;

JavaVM *g_jvm = NULL;
jobject g_obj = NULL;


#ifdef __cplusplus
extern "C" {
#endif

void updateVideoData(YUVData_Frame* yuvFrame,unsigned long userData);

void Java_com_example_jcmplayer_JCMPlayer_ndkInitVideoPlayer(JNIEnv *env, jobject obj)
{
    m_avCodecHandler.SetupUpdateVideoCallback(updateVideoData,NULL);
    env->GetJavaVM(&g_jvm);
    g_obj=env->NewGlobalRef(obj);

}



void Java_com_example_jcmplayer_JCMPlayer_ndkStartPlayerWithFile(JNIEnv *env, jobject obj, jstring fileName)
{
    std::string fileString = env->GetStringUTFChars(fileName,0);
    LOGD("PATH STRING: %s",fileString.c_str());

    m_avCodecHandler.StopPlayVideo();
    m_avCodecHandler.SetVideoFilePath(fileString);
    m_avCodecHandler.InitVideoCodec();
    m_avCodecHandler.StartPlayVideo();
}

void Java_com_example_jcmplayer_JCMPlayer_ndkPauseVideoPlay(JNIEnv *env, jobject obj)
{

}

void Java_com_example_jcmplayer_JCMPlayer_ndkStopVideoPlayer(JNIEnv *env, jobject obj)
{
    m_avCodecHandler.StopPlayVideo();
}

jfloat Java_com_example_jcmplayer_JCMPlayer_ndkGetVideoSizeRatio(JNIEnv *env, jobject obj)
{
    int vWidth = m_avCodecHandler.GetVideoWidth();
    int vHeight = m_avCodecHandler.GetVideoHeight();
    jfloat  ratio = (jfloat)vWidth / (jfloat)vHeight;
    LOGD("VIDEO SIZE RATIO: %f",ratio);
    return ratio;
}
jfloat Java_com_example_jcmplayer_JCMPlayer_ndkGetVideoTotalSeconds(JNIEnv *env, jobject obj)
{
    jfloat  totalSeconds = m_avCodecHandler.GetMediaTotalSeconds();
    return totalSeconds;
}

void Java_com_example_jcmplayer_JCMPlayer_ndkSeekMedia(JNIEnv *env, jobject obj, jfloat posValue)
{
    m_avCodecHandler.SeekMedia(posValue);
}

void Java_com_example_jcmplayer_JCMPlayer_ndkInitGL(JNIEnv *env, jobject obj,jobject assetManager)
{
    AAssetManager *astManager = AAssetManager_fromJava (env, assetManager);
    if (NULL != astManager){
        m_glVideoRender.SetupAssetManager(astManager);
    }
    m_glVideoRender.InitGL();
}

void Java_com_example_jcmplayer_JCMPlayer_ndkPaintGL(JNIEnv *env, jobject obj)
{
    m_glVideoRender.PaintGL();
}

void Java_com_example_jcmplayer_JCMPlayer_ndkResizeGL(JNIEnv *env, jobject obj, jint width, jint height)
{
    m_glVideoRender.ResizeGL(width,height);
}

void updateVideoData(YUVData_Frame* yuvFrame,unsigned long userData)
{
    if(yuvFrame == NULL){
        return;
    }

    m_glVideoRender.RendVideo(yuvFrame);

    //LOGD("UPDATE VIDEO DATA: %d %d",yuvFrame->width,yuvFrame->height);
    JNIEnv *m_env;
    jmethodID m_methodID;
    jclass m_class;
    //Attach主线程
    if(g_jvm->AttachCurrentThread(&m_env, NULL) != JNI_OK)
    {
        LOGF("JVM: AttachCurrentThread failed");
        return ;
    }
    //找到对应的类
    m_class = m_env->GetObjectClass(g_obj);
    if(m_class == NULL)
    {
        LOGD("JVM: FindClass error.....");
        if(g_jvm->DetachCurrentThread() != JNI_OK)
        {
            LOGF("JVM: DetachCurrentThread failed");
        }
        return ;
    }
    //再获得类中的方法
    m_methodID = m_env->GetMethodID(m_class, "OnVideoRenderCallback", "()V");
    if (m_methodID == NULL)
    {
        LOGF("JVM: GetMethodID error.....");
        //Detach主线程
        if(g_jvm->DetachCurrentThread() != JNI_OK)
        {
            LOGF("JVM: DetachCurrentThread failed");
        }
    }


    m_env->CallVoidMethod(g_obj, m_methodID );
    m_env->DeleteLocalRef(m_class);

}



#ifdef __cplusplus
}
#endif