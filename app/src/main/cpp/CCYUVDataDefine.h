#ifndef CCYUVDATADEFINE_H
#define CCYUVDATADEFINE_H

#include <stdint.h>
#include <stdio.h>

#pragma pack(push, 1)

#define MAX_AUDIO_FRAME_IN_QUEUE    1200
#define MAX_VIDEO_FRAME_IN_QUEUE    600

typedef struct YUVChannelDef
{
    unsigned int    length;
    unsigned char*  dataBuffer;

}YUVChannel;

typedef struct  YUVFrameDef
{
    unsigned int    width;
    unsigned int    height;
    YUVChannel      luma;
    YUVChannel      chromaB;
    YUVChannel      chromaR;
    long long       pts;

}YUVData_Frame;

typedef struct DecodedAudiodataDef
{
    unsigned char*  dataBuff;
    unsigned int    dataLength;
}JCDecodedAudioData;


#pragma pack(pop)

#endif // CCYUVDATADEFINE_H
