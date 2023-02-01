

#ifndef JCMPLAYER_CCGLVIDEORENDER_H
#define JCMPLAYER_CCGLVIDEORENDER_H

#include "CCOpenGLShader.h"
#include "CCNDKCommonDef.h"
#include "CCYUVDataDefine.h"

class CCGLVideoRender {

public:
    CCGLVideoRender();
    ~CCGLVideoRender();

    void InitGL();
    void PaintGL();
    void ResizeGL(int w,int h);

    void SetupAssetManager(AAssetManager *pManager);

    void RendVideo(YUVData_Frame * frame);

private:
    void loadShaderResources(AAssetManager *pManager);

private:

    AAssetManager*        m_pAssetManager;
    CCOpenGLShader*       m_pOpenGLShader;

    bool   m_bUpdateData = false;

    GLuint          m_textures[3];


    int m_nVideoW       =0; //视频分辨率宽
    int m_nVideoH       =0; //视频分辨率高
    int m_yFrameLength  =0;
    int m_uFrameLength  =0;
    int m_vFrameLength  =0;

    unsigned char* m_pBufYuv420p = NULL;

    struct CCVertex{
        float x,y,z;
        float u,v;
    };

};


#endif //JCMPLAYER_CCGLVIDEORENDER_H
