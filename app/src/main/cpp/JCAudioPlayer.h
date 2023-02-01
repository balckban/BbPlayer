

#ifndef JCMPLAYER_JCAUDIOPLAYER_H
#define JCMPLAYER_JCAUDIOPLAYER_H
#include <mutex>
#include "OpenSLInterface.h"

#define OUT_BUFFER_SIZE 8192

class JCAudioPlayer {

public:

    static JCAudioPlayer* GetInstance();

    void StartAudioPlayer();
    void StopAudioPlayer();

    int GetFreeSpace();
    bool WriteAudioData(const char* dataBuff,int size);

    void SetSampleRate(int value){m_sampleRate = value;}
    void SetSampleSize(int value){m_sampleSize = value;}
    void Setchannel(int value){m_channel = value;}

private:

    JCAudioPlayer();

    int m_sampleRate = 44100;
    int m_sampleSize = 16;
    int m_channel = 2;

    static JCAudioPlayer*  m_pInstance;
    static pthread_mutex_t m_mutex;

    OPENSL_STREAM*  m_stream;

    class Garbage
    {
    public:
        ~Garbage(){
            if(JCAudioPlayer::m_pInstance){
                delete JCAudioPlayer::m_pInstance;
                JCAudioPlayer::m_pInstance = NULL;
            }
        }
    };
    static Garbage  m_garbage;

};


#endif //JCMPLAYER_JCAUDIOPLAYER_H
