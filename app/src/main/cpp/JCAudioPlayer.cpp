//
// Created by chenchao on 2021/8/21.
//

#include "JCAudioPlayer.h"
#include "CCNDKLogDef.h"

JCAudioPlayer* JCAudioPlayer::m_pInstance = NULL;
pthread_mutex_t JCAudioPlayer::m_mutex;
JCAudioPlayer::Garbage JCAudioPlayer::m_garbage;

JCAudioPlayer* JCAudioPlayer::GetInstance()
{
    if(m_pInstance == nullptr){
        pthread_mutex_init(&m_mutex,NULL);

        pthread_mutex_lock(&m_mutex);
        m_pInstance = new JCAudioPlayer();
        pthread_mutex_unlock(&m_mutex);
    }

    return m_pInstance;
}

JCAudioPlayer::JCAudioPlayer()
{
    m_stream=NULL;
}


void JCAudioPlayer::StartAudioPlayer()
{
    pthread_mutex_lock(&m_mutex);

    StopAudioPlayer();
    m_stream = android_OpenAudioDevice(m_sampleRate, m_channel, m_channel, OUT_BUFFER_SIZE);

    pthread_mutex_unlock(&m_mutex);
}

void JCAudioPlayer::StopAudioPlayer()
{
    if(m_stream != NULL){
        pthread_mutex_lock(&m_mutex);
        android_CloseAudioDevice(m_stream);
        m_stream = NULL;
        pthread_mutex_unlock(&m_mutex);
    }

}

int JCAudioPlayer::GetFreeSpace()
{
    pthread_mutex_lock(&m_mutex);
    return m_stream->outBufSamples;
    pthread_mutex_unlock(&m_mutex);
}


bool JCAudioPlayer::WriteAudioData(const char* dataBuff,int size)
{

    if((dataBuff == NULL) || (size <= 0)){
        return false;
    }

    pthread_mutex_lock(&m_mutex);
    int writeSamples = android_AudioOut(m_stream,(short*)dataBuff,size/2);
    pthread_mutex_unlock(&m_mutex);
    //LOGD("WRITE SAMPLES:%d %d",size,writeSamples);
    return true;
}

