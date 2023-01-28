//
// Created by chenchao on 2021/8/20.
//

#include "JCAVCodecHandler.h"
#include "CCNDKLogDef.h"
#include "JCAudioPlayer.h"

#if !defined(MIN)
#define MIN(A,B)	((A) < (B) ? (A) : (B))
#endif

std::atomic<bool>   m_bFileThreadRunning(false);
std::atomic<bool>   m_bAudioThreadRunning(false);
std::atomic<bool>   m_bVideoThreadRunning(false);
std::atomic<bool>   m_bThreadRunning(false);

JCAVCodecHandler::JCAVCodecHandler()
{
    av_register_all();

    resetAllMediaPlayerParameters();

}

JCAVCodecHandler::~JCAVCodecHandler()
{
}

void JCAVCodecHandler::SetVideoFilePath(std::string& path)
{
    m_videoPathString = path;

    std::string fileSuffix = getFileSuffix(m_videoPathString.c_str());
    LOGD("File suffix %s",fileSuffix.c_str());
    if(fileSuffix == "mp3"){
        m_mediaType = MEDIATYPE_MUSIC;
    }else{
        m_mediaType = MEDIATYPE_VIDEO;
    }
}

std::string JCAVCodecHandler::GetVideoFilePath()
{
    return m_videoPathString;
}

int JCAVCodecHandler::GetVideoWidth()
{
    return m_videoWidth;
}

int JCAVCodecHandler::GetVideoHeight()
{
    return m_videoHeight;
}

void JCAVCodecHandler::SetupUpdateVideoCallback(UpdateVideo2GUI_Callback callback,unsigned long userData)
{

    m_updateVideoCallback = callback;
    m_userDataVideo=userData;
}

void JCAVCodecHandler::SetupUpdateCurrentPTSCallback(UpdateCurrentPTS_Callback callback,unsigned long userData)
{

    m_updateCurrentPTSCallback = callback;
    m_userDataPts=userData;
}

// 市面上的音视频有几种情况 完善的播放器,需要做处理。
// 1 有视频，有音频 m_pVideoCodecCtx != NULL m_pAudioCodecCtx != NULL 用音频同步视频
// 2 有视频，没音频 m_pVideoCodecCtx != NULL m_pAudioCodecCtx == NULL 按照视频FPS自己播放
// 3 有封面的mp3 同1 但只有一张封面图片，当作视频，相当于只有一帧视频.
// 4 没有封面的mp3 盗版音乐 m_pVideoCodecCtx == NULL m_pAudioCodecCtx != NULL

int JCAVCodecHandler::InitVideoCodec()
{

    if(m_videoPathString == ""){
        LOGF("file path is Empty...");
        return -1;
    }

    const char * filePath=m_videoPathString.c_str();

    if (avformat_open_input(&m_pFormatCtx, filePath, NULL, NULL) != 0)
    {
        LOGF("open input stream failed .");
        return -1;
    }

    if (avformat_find_stream_info(m_pFormatCtx, nullptr) < 0)
    {
        LOGF("avformat_find_stream_info failed .");
        return -1;
    }

    av_dump_format(m_pFormatCtx, 0, filePath, 0);

    m_videoStreamIdx = -1;
    m_audioStreamIdx = -1;


    for (int i = 0; i < (int)m_pFormatCtx->nb_streams; ++i)
    {

        AVCodecParameters  *codecParameters = m_pFormatCtx->streams[i]->codecpar;

        if (codecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            m_videoStreamIdx = i;
            LOGF("video index: %d",m_videoStreamIdx);

            AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
            if (codec == nullptr)
            {
                LOGF("Video AVCodec is NULL");
                return -1;
            }

            m_pVideoCodecCtx = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(m_pVideoCodecCtx, codecParameters);

            if (avcodec_open2(m_pVideoCodecCtx, codec, NULL) < 0)
            {
                LOGF("Could not open codec.");
                return -1;
            }

        }
        else if (codecParameters->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            m_audioStreamIdx = i;
            LOGF("Audio index: %d",m_audioStreamIdx);


            AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
            if (codec == nullptr)
            {
                LOGF("Audo AVCodec is NULL");
                return -1;
            }

            m_pAudioCodecCtx = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(m_pAudioCodecCtx, codecParameters);

            if (avcodec_open2(m_pAudioCodecCtx, codec, NULL) < 0)
            {
                LOGF("audio decoder not found.");
                return -1;
            }

            m_sampleRate = m_pAudioCodecCtx->sample_rate;
            m_channel = m_pAudioCodecCtx->channels;
            switch (m_pAudioCodecCtx->sample_fmt)
            {
                case AV_SAMPLE_FMT_U8:
                    m_sampleSize = 8;
                case AV_SAMPLE_FMT_S16:
                    m_sampleSize = 16;
                    break;
                case AV_SAMPLE_FMT_S32:
                    m_sampleSize = 32;
                    break;
                default:
                    break;
            }
        }
    }

    JCAudioPlayer::GetInstance()->SetSampleRate(m_sampleRate);
    JCAudioPlayer::GetInstance()->SetSampleSize(m_sampleSize);
    JCAudioPlayer::GetInstance()->Setchannel(m_channel);


    if(m_pVideoCodecCtx != NULL)
    {
        m_pYUVFrame = av_frame_alloc();
        if (m_pYUVFrame == NULL)
        {
            LOGF("YUV frame alloc error.");
            return -1;
        }

        m_pYUV420Buffer = (uint8_t *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, m_pVideoCodecCtx->width, m_pVideoCodecCtx->height, 1));

        if (m_pYUVFrame == NULL)
        {
            LOGF("out buffer alloc error.");
            return -1;
        }

        av_image_fill_arrays(m_pYUVFrame->data, m_pYUVFrame->linesize, m_pYUV420Buffer, AV_PIX_FMT_YUV420P, m_pVideoCodecCtx->width, m_pVideoCodecCtx->height, 1);


        m_videoWidth  = m_pVideoCodecCtx->width;
        m_videoHeight = m_pVideoCodecCtx->height;

        LOGD("Init VideoCodec sucess. Width: %d Height: %d",m_videoWidth,m_videoHeight);
    }

    if(m_videoStreamIdx != -1){
        AVStream *videoStream=m_pFormatCtx->streams[m_videoStreamIdx];
        m_vStreamTimeRational = videoStream->time_base;
        m_videoFPS            = videoStream->avg_frame_rate.num / videoStream->avg_frame_rate.den;
        LOGD("Init VideoCodec sucess.V Time Base: %d  Den :%d",videoStream->time_base.num,videoStream->time_base.den);
    }

    if(m_audioStreamIdx != -1){
        AVStream *audioStream=m_pFormatCtx->streams[m_audioStreamIdx];
        m_aStreamTimeRational = audioStream->time_base;
        LOGD("Init VideoCodec sucess.A Time Base: %d Den  %d",audioStream->time_base.num,audioStream->time_base.den);
    }


    return 0;

}


void JCAVCodecHandler::StartPlayVideo()
{

    JCAudioPlayer::GetInstance()->StartAudioPlayer();

    startMediaProcessThreads();
}

void JCAVCodecHandler::StopPlayVideo()
{
    m_bThreadRunning = false;

    m_bReadFileEOF = false;

    m_nCurrAudioTimeStamp =0.0f;
    m_nLastAudioTimeStamp =0.0f;

    m_vStreamTimeRational = av_make_q(0,0);
    m_aStreamTimeRational = av_make_q(0,0);

    m_eMediaPlayStatus = MEDIAPLAY_STATUS_STOP;

    JCAudioPlayer::GetInstance()->StopAudioPlayer();

    while ( !m_audioPktQueue.isEmpty() )
        freePacket( m_audioPktQueue.dequeue() );
    while ( !m_videoPktQueue.isEmpty() )
        freePacket( m_videoPktQueue.dequeue() );

    waitAllThreadsExit();

    UnInitVideoCodec();

    resetAllMediaPlayerParameters();

    LOGD("======STOP PLAY VIDEO SUCCESS========");
}



void JCAVCodecHandler::startMediaProcessThreads()
{

    m_bThreadRunning =true;

    std::thread readThread(&JCAVCodecHandler::doReadMediaFrameThread,this);
    readThread.detach();

    std::thread audioThread(&JCAVCodecHandler::doAudioDecodePlayThread,this);
    audioThread.detach();

    std::thread videoThread(&JCAVCodecHandler::doVideoDecodeShowThread,this);
    videoThread.detach();


}

void JCAVCodecHandler::SeekMedia(float nPos)
{
    if(nPos <0 ){
        return;
    }
    if(m_pFormatCtx == nullptr){
        return;
    }

    m_bThreadRunning = false;
    m_bReadFileEOF = false;

    if(m_mediaType == MEDIATYPE_VIDEO){

        m_nSeekingPos =(long long) nPos / av_q2d(m_vStreamTimeRational);

        if( m_audioStreamIdx >= 0 && m_videoStreamIdx >= 0 ){
            av_seek_frame(m_pFormatCtx, m_videoStreamIdx, m_nSeekingPos, AVSEEK_FLAG_BACKWARD );
        }
    }

    if(m_mediaType == MEDIATYPE_MUSIC){
        LOGD("SEEK AUDIO");
        m_nSeekingPos =(int64) nPos / av_q2d(m_aStreamTimeRational);
        if( m_audioStreamIdx >= 0  ){
            av_seek_frame(m_pFormatCtx, m_audioStreamIdx, m_nSeekingPos, AVSEEK_FLAG_ANY );
        }
    }


    while ( !m_videoPktQueue.isEmpty() )
    {
        freePacket( m_videoPktQueue.dequeue() );
    }

    while ( !m_audioPktQueue.isEmpty() )
    {
        freePacket( m_audioPktQueue.dequeue());
    }



    waitAllThreadsExit();

    startMediaProcessThreads();

}

void JCAVCodecHandler::waitAllThreadsExit()
{
    while(m_bFileThreadRunning){
        stdThreadSleep(10);
        continue;
    }

    while(m_bAudioThreadRunning){
        stdThreadSleep(10);
        continue;
    }

    while(m_bVideoThreadRunning){
        stdThreadSleep(10);
        continue;
    }
}


float JCAVCodecHandler::GetMediaTotalSeconds()
{
    float totalDuration = m_pFormatCtx->duration/(AV_TIME_BASE*1.000);
    return totalDuration;
}

void JCAVCodecHandler::SetMediaStatusPlay()
{
    m_eMediaPlayStatus = MEDIAPLAY_STATUS_PLAYING;
}

void JCAVCodecHandler::SetMediaStatusPause()
{
    m_eMediaPlayStatus = MEDIAPLAY_STATUS_PAUSE;
}

int JCAVCodecHandler::GetPlayerStatus()
{
    return m_eMediaPlayStatus;
}

void JCAVCodecHandler::tickVideoFrameTimerDelay(int64_t pts)
{
    if(m_vStreamTimeRational.den <=0){
        return;
    }

    float currVideoTimeStamp=pts* av_q2d(m_vStreamTimeRational);

    if(m_pAudioCodecCtx == NULL){

        float sleepTime= 1000.0 / (float)m_videoFPS;
        stdThreadSleep((int)sleepTime);

        if(m_updateCurrentPTSCallback){
            m_updateCurrentPTSCallback(currVideoTimeStamp,m_userDataPts);
        }

        return;
    }


    float diffTime = (currVideoTimeStamp - m_nCurrAudioTimeStamp)*1000;

    int sleepTime= (int)diffTime;

    //LOGD("A TimeStamp: %f",m_nCurrAudioTimeStamp);
    //LOGD("AT: %f VT: %f ST: %d ",m_nCurrAudioTimeStamp,currVideoTimeStamp,sleepTime);

    if(sleepTime > 0 && sleepTime < 5000 ){
        stdThreadSleep(sleepTime);
    }
    else{
        //stdThreadSleep(0);
    }

}


void JCAVCodecHandler::tickAudioFrameTimerDelay(int64_t pts)
{
    if(m_aStreamTimeRational.den <=0){
        return;
    }

    m_nCurrAudioTimeStamp=pts* av_q2d(m_aStreamTimeRational);

    int diffTime= (int)(m_nCurrAudioTimeStamp - m_nLastAudioTimeStamp);

    if(abs(diffTime) >= 1)
    {
        if(m_updateCurrentPTSCallback){
            m_updateCurrentPTSCallback(m_nCurrAudioTimeStamp,m_userDataPts);
        }

        m_nLastAudioTimeStamp = m_nCurrAudioTimeStamp;
    }

    return;
}


void JCAVCodecHandler::doReadMediaFrameThread()
{

    while (m_bThreadRunning)
    {
        m_bFileThreadRunning = true;

        if(m_eMediaPlayStatus == MEDIAPLAY_STATUS_PAUSE){
            stdThreadSleep(10);
            continue;
        }

        if(m_pVideoCodecCtx != NULL && m_pAudioCodecCtx != NULL){ //有视频,有音频

            if( m_videoPktQueue.size() > MAX_VIDEO_FRAME_IN_QUEUE && m_audioPktQueue.size() > MAX_AUDIO_FRAME_IN_QUEUE)
            {
                stdThreadSleep(10);
                continue;
            }

        }
        else if(m_pVideoCodecCtx != NULL && m_pAudioCodecCtx == NULL){ //只有视频,没有音频

            float sleepTime= 1000.0 / (float)m_videoFPS;
            stdThreadSleep((int)sleepTime);

        }
        else{//只有音频
            if( m_videoPktQueue.size() > MAX_AUDIO_FRAME_IN_QUEUE )
            {
                stdThreadSleep(10);
                continue;
            }
        }

        //qDebug()<<"QUEUE"<< m_videoPktQueue.size() << m_audioPktQueue.size()<<m_bReadFileEOF;
        if(m_bReadFileEOF == false){
            readMediaPacket();
        }
        else{
            //m_bThreadRunning = false;
            stdThreadSleep(10);
        }

    }

    LOGD("read file thread exit...");

    m_bFileThreadRunning = false;

    return;
}



void  JCAVCodecHandler::readMediaPacket()
{
    AVPacket *packet = (AVPacket*)malloc( sizeof(AVPacket) );
    if ( !packet ) {
        return;
    }

    av_init_packet( packet );

    m_eMediaPlayStatus = MEDIAPLAY_STATUS_PLAYING;

    int retValue = av_read_frame(m_pFormatCtx, packet);
    if (retValue == 0)
    {
        if(packet->stream_index == m_videoStreamIdx) //Video frame
        {
            if ( !av_dup_packet( packet ) ){

                m_videoPktQueue.enqueue(packet);
            }
            else{
                freePacket( packet );
            }

        }
        if(packet->stream_index == m_audioStreamIdx) //Video frame
        {
            if ( !av_dup_packet( packet ) ){

                m_audioPktQueue.enqueue(packet);
            }
            else{
                freePacket( packet );
            }
        }
    }
    else if(retValue <0)
    {
        if((m_bReadFileEOF == false) && (retValue == AVERROR_EOF)){
            m_bReadFileEOF = true;
        }
        return;
    }

}

float JCAVCodecHandler::getVideoTimeStampFromPTS(int64 pts)
{
    float vTimeStamp=pts* av_q2d(m_vStreamTimeRational);
    return vTimeStamp;
}

float JCAVCodecHandler::getAudioTimeStampFromPTS(int64 pts)
{
    float aTimeStamp=pts* av_q2d(m_aStreamTimeRational);
    return aTimeStamp;
}

void JCAVCodecHandler::doVideoDecodeShowThread()
{
    if(m_pFormatCtx == NULL){
        return;
    }

    if (m_pVideoFrame == NULL)
    {
        m_pVideoFrame = av_frame_alloc();
    }

    while(m_bThreadRunning)
    {

        m_bVideoThreadRunning = true;

        if(m_eMediaPlayStatus == MEDIAPLAY_STATUS_PAUSE){
            stdThreadSleep(10);
            continue;
        }

        if(m_videoPktQueue.isEmpty()){
            stdThreadSleep(1);
            continue;
        }

        AVPacket* pkt = (AVPacket*)m_videoPktQueue.dequeue();
        if(pkt == NULL){
            break;
        }

        if(!m_bThreadRunning){
            freePacket( pkt);
            break;
        }

        tickVideoFrameTimerDelay(pkt->pts);

        int retValue = avcodec_send_packet(m_pVideoCodecCtx, pkt);
        if (retValue != 0)
        {
            freePacket( pkt);

            continue;
        }

        int decodeRet  = avcodec_receive_frame(m_pVideoCodecCtx, m_pVideoFrame);
        if (decodeRet == 0)
        {
            convertAndRenderVideo(m_pVideoFrame,pkt->pts);
        }


        freePacket( pkt);
    }

    LOGD("video decode show  thread exit...");

    m_bVideoThreadRunning = false;

    return;

}

void JCAVCodecHandler::doAudioDecodePlayThread()
{
    if(m_pFormatCtx == NULL){
        return;
    }


    if(m_pAudioFrame == NULL)
    {
        m_pAudioFrame = av_frame_alloc();
    }


    while(m_bThreadRunning)
    {
        m_bAudioThreadRunning = true;

        if(m_eMediaPlayStatus == MEDIAPLAY_STATUS_PAUSE){
            stdThreadSleep(10);
            continue;
        }

        if(m_audioPktQueue.isEmpty()){
            stdThreadSleep(1);
            continue;
        }


        AVPacket* pkt = (AVPacket*)m_audioPktQueue.dequeue();
        if(pkt == NULL){
            break;
        }

        if(!m_bThreadRunning){
            freePacket( pkt);
            break;
        }

        tickAudioFrameTimerDelay(pkt->pts);


        int retValue = avcodec_send_packet(m_pAudioCodecCtx, pkt);
        if (retValue != 0)
        {
            freePacket( pkt);

            continue;
        }


        int decodeRet = avcodec_receive_frame(m_pAudioCodecCtx, m_pAudioFrame);
        if (decodeRet == 0)
        {
            convertAndPlayAudio(m_pAudioFrame);
        }


        freePacket( pkt);

    }

    LOGD("audio decode show  thread exit...");

    m_bAudioThreadRunning = false;

    return;
}
void JCAVCodecHandler::convertAndPlayAudio(AVFrame* decodedFrame)
{
    if (!m_pFormatCtx || !m_pAudioFrame || !decodedFrame)
    {
        return ;
    }

    if (m_pAudioSwrCtx == NULL)
    {
        m_pAudioSwrCtx = swr_alloc();

        swr_alloc_set_opts(m_pAudioSwrCtx, av_get_default_channel_layout(m_channel),
                           AV_SAMPLE_FMT_S16,
                           m_sampleRate,
                           av_get_default_channel_layout(m_pAudioCodecCtx->channels),
                           m_pAudioCodecCtx->sample_fmt,
                           m_pAudioCodecCtx->sample_rate,
                           0, NULL);

        if (m_pAudioSwrCtx !=NULL){
            int ret=swr_init(m_pAudioSwrCtx);
            //qDebug()<<"swr_init RetValue:"<<ret;
        }

    }


    int bufSize = av_samples_get_buffer_size(NULL, m_channel, decodedFrame->nb_samples, AV_SAMPLE_FMT_S16, 0);

    if (!m_pSwrBuffer || m_swrBuffSize < bufSize) {
        m_swrBuffSize = bufSize;
        m_pSwrBuffer = (uint8_t *)realloc(m_pSwrBuffer, m_swrBuffSize);
    }


    uint8_t *outbuf[2] = { m_pSwrBuffer, 0 };

    int len = swr_convert(m_pAudioSwrCtx, outbuf, decodedFrame->nb_samples, (const uint8_t **)decodedFrame->data, decodedFrame->nb_samples);
    if (len <= 0)
    {
        return ;
    }


    if( !m_bThreadRunning ){
        return;
    }


    JCAudioPlayer::GetInstance()->WriteAudioData((const char*)m_pSwrBuffer,bufSize);


}

void JCAVCodecHandler::convertAndRenderVideo(AVFrame* videoFrame,long long ppts)
{
    if(videoFrame == NULL){
        return;
    }


    if(m_pVideoSwsCtx == NULL)
    {
        //获取sws上下文
        m_pVideoSwsCtx = sws_getContext(m_pVideoCodecCtx->width,m_pVideoCodecCtx->height,
                                        m_pVideoCodecCtx->pix_fmt,
                                        m_pVideoCodecCtx->width,m_pVideoCodecCtx->height,
                                        AV_PIX_FMT_YUV420P,
                                        SWS_BICUBIC,NULL,NULL,NULL);
    }

    sws_scale(m_pVideoSwsCtx, (const uint8_t* const*)videoFrame->data,
              videoFrame->linesize, 0,
              m_pVideoCodecCtx->height,
              m_pYUVFrame->data,
              m_pYUVFrame->linesize);



    unsigned int lumaLength= m_pVideoCodecCtx->height * (MIN(videoFrame->linesize[0], m_pVideoCodecCtx->width));
    unsigned int chromBLength=((m_pVideoCodecCtx->height)/2)*(MIN(videoFrame->linesize[1], (m_pVideoCodecCtx->width)/2));
    unsigned int chromRLength=((m_pVideoCodecCtx->height)/2)*(MIN(videoFrame->linesize[2], (m_pVideoCodecCtx->width)/2));

    YUVData_Frame *updateYUVFrame = new YUVData_Frame();

    updateYUVFrame->luma.length = lumaLength;
    updateYUVFrame->chromaB.length = chromBLength;
    updateYUVFrame->chromaR.length =chromRLength;

    updateYUVFrame->luma.dataBuffer=(unsigned char*)malloc(lumaLength);
    updateYUVFrame->chromaB.dataBuffer=(unsigned char*)malloc(chromBLength);
    updateYUVFrame->chromaR.dataBuffer=(unsigned char*)malloc(chromRLength);


    copyDecodedFrame420(m_pYUVFrame->data[0],updateYUVFrame->luma.dataBuffer,m_pYUVFrame->linesize[0],
                        m_pVideoCodecCtx->width,m_pVideoCodecCtx->height);
    copyDecodedFrame420(m_pYUVFrame->data[1], updateYUVFrame->chromaB.dataBuffer,m_pYUVFrame->linesize[1],
                        m_pVideoCodecCtx->width / 2,m_pVideoCodecCtx->height / 2);
    copyDecodedFrame420(m_pYUVFrame->data[2], updateYUVFrame->chromaR.dataBuffer,m_pYUVFrame->linesize[2],
                        m_pVideoCodecCtx->width / 2,m_pVideoCodecCtx->height / 2);

    updateYUVFrame->width=m_pVideoCodecCtx->width;
    updateYUVFrame->height=m_pVideoCodecCtx->height;

    updateYUVFrame->pts = ppts;

    if(m_updateVideoCallback)
    {
        m_updateVideoCallback(updateYUVFrame,m_userDataVideo);
    }

    if(updateYUVFrame->luma.dataBuffer){
        free(updateYUVFrame->luma.dataBuffer);
        updateYUVFrame->luma.dataBuffer=NULL;
    }

    if(updateYUVFrame->chromaB.dataBuffer){
        free(updateYUVFrame->chromaB.dataBuffer);
        updateYUVFrame->chromaB.dataBuffer=NULL;
    }

    if(updateYUVFrame->chromaR.dataBuffer){
        free(updateYUVFrame->chromaR.dataBuffer);
        updateYUVFrame->chromaR.dataBuffer=NULL;
    }

    if(updateYUVFrame){
        delete updateYUVFrame;
        updateYUVFrame = NULL;
    }

}


void JCAVCodecHandler::copyDecodedFrame420(uint8_t* src, uint8_t* dist,int linesize, int width, int height)
{

    width = MIN(linesize, width);
    for (int i = 0; i < height; ++i) {
        memcpy(dist, src, width);
        dist += width;
        src += linesize;
    }

}

void JCAVCodecHandler::copyDecodedFrame(uint8_t* src, uint8_t* dist,int linesize, int width, int height)
{

    width = MIN(linesize, width);

    for (int i = 0; i < height; ++i) {
        memcpy(dist, src, width);
        dist += width;
        src += linesize;
    }

}

void JCAVCodecHandler::UnInitVideoCodec()
{
    LOGD("UnInitVideoCodec...");

    if (m_pSwrBuffer != NULL) {

        free(m_pSwrBuffer);
    }

    if(m_pAudioSwrCtx != NULL)
    {
        swr_free(&m_pAudioSwrCtx);
    }



    if(m_pAudioFrame != NULL)
    {
        av_frame_free(&m_pAudioFrame);
    }

    if(m_pAudioCodecCtx != NULL)
    {
        avcodec_close(m_pAudioCodecCtx);
    }




    if(m_pYUV420Buffer != NULL ){
        av_free(m_pYUV420Buffer);
    }

    if(m_pYUVFrame != NULL)
    {
        av_frame_free(&m_pYUVFrame);
    }

    if(m_pVideoSwsCtx  != NULL)
    {
        sws_freeContext(m_pVideoSwsCtx );
    }



    if(m_pVideoFrame != NULL)
    {
        av_frame_free(&m_pVideoFrame);
    }


    if(m_pVideoCodecCtx != NULL)
    {
        avcodec_close(m_pVideoCodecCtx);
    }



    if(m_pFormatCtx != NULL)
    {
        avformat_close_input(&m_pFormatCtx);
    }

}

void JCAVCodecHandler::freePacket(AVPacket* pkt)
{
    if(pkt == NULL ){
        return;
    }

    av_free_packet( pkt );

    free( pkt );
}

void JCAVCodecHandler::stdThreadSleep(int mseconds)
{
    std::chrono::milliseconds sleepTime(mseconds);
    std::this_thread::sleep_for(sleepTime);
}

void JCAVCodecHandler::resetAllMediaPlayerParameters()
{

    m_pFormatCtx       = NULL;
    m_pVideoCodecCtx   = NULL;
    m_pAudioCodecCtx   = NULL;
    m_pYUVFrame        = NULL;
    m_pVideoFrame      = NULL;
    m_pAudioFrame      = NULL;
    m_pAudioSwrCtx     = NULL;
    m_pVideoSwsCtx     = NULL;
    m_pYUV420Buffer    = NULL;
    m_pSwrBuffer       = NULL;

    m_videoWidth   = 0;
    m_videoHeight  = 0;

    m_videoPathString = "";

    m_videoStreamIdx = -1;
    m_audioStreamIdx = -1;

    m_bReadFileEOF   = false;

    m_nSeekingPos      = 0;


    m_nCurrAudioTimeStamp = 0.0f;
    m_nLastAudioTimeStamp = 0.0f;

    m_sampleRate = 48000;
    m_sampleSize = 16;
    m_channel    = 2;

    m_volumeRatio = 1.0f;
    m_swrBuffSize = 0;

    m_vStreamTimeRational = av_make_q(0,0);
    m_aStreamTimeRational = av_make_q(0,0);

    m_mediaType = MEDIATYPE_VIDEO;
    m_eMediaPlayStatus = MEDIAPLAY_STATUS_STOP;
}
std::string JCAVCodecHandler::getFileSuffix(const char* path)
{
    const char* pos = strrchr(path,'.');
    if(pos)
    {
        std::string str(pos+1);
        std::transform(str.begin(),str.end(),str.begin(),::tolower);
        return str;
    }
    return std::string();
}