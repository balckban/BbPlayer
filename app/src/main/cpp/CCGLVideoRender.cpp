//
// Created by chenchao on 2021/8/22.
//

#include "CCGLVideoRender.h"


CCGLVideoRender::CCGLVideoRender()
{
    m_pOpenGLShader = new CCOpenGLShader();
}

CCGLVideoRender::~CCGLVideoRender()
{
    CCGLSafePtrDelete( m_pOpenGLShader);
}

void CCGLVideoRender::InitGL()
{
    m_bUpdateData = false;

    glClearColor(0.0,0.0,0.0,1.0);
    glClearDepthf(1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glGenTextures(3, m_textures);

    if(m_pAssetManager != NULL){
        loadShaderResources(m_pAssetManager);
    }

    return;
}

void CCGLVideoRender::PaintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    if(m_bUpdateData == false){
        return;
    }

    static CCVertex triangleVert[] = {
            {-1, 1,  1,     0,0},
            {-1, -1,  1,    0,1},
            {1,  1,  1,     1,0},
            {1,  -1,  1,    1,1},
    };


    glm::mat4x4  objectMat;
    glm::mat4x4  objectTransMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3));
    glm::mat4 projMat = glm::ortho(-1.0,1.0,-1.0,1.0,0.1,1000.0);

    objectMat = projMat* objectTransMat ;


    m_pOpenGLShader->Bind();

    m_pOpenGLShader->SetUniformValue("uni_mat",objectMat);


    m_pOpenGLShader->EnableAttributeArray("attr_position");
    m_pOpenGLShader->EnableAttributeArray("attr_uv");

    m_pOpenGLShader->SetAttributeBuffer("attr_position",GL_FLOAT,triangleVert,3,sizeof(CCVertex));
    m_pOpenGLShader->SetAttributeBuffer("attr_uv",GL_FLOAT,&triangleVert[0].u,2,sizeof(CCVertex));


    m_pOpenGLShader->SetUniformValue("uni_textureY",0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textures[0]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_nVideoW, m_nVideoH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_pBufYuv420p);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_pOpenGLShader->SetUniformValue("uni_textureU",1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_textures[1]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,m_nVideoW/2, m_nVideoH/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (char*)(m_pBufYuv420p+m_yFrameLength));
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_pOpenGLShader->SetUniformValue("uni_textureV",2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_textures[2]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_nVideoW/2, m_nVideoH/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (char*)(m_pBufYuv420p+m_yFrameLength+m_uFrameLength));
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


    m_pOpenGLShader->DisableAttributeArray("attr_position");
    m_pOpenGLShader->DisableAttributeArray("attr_uv");

    m_pOpenGLShader->Release();

    return;
}

void CCGLVideoRender::RendVideo(YUVData_Frame * yuvFrame)
{
    if(yuvFrame == NULL ){
        return;
    }

    if(m_nVideoH != yuvFrame->height || m_nVideoW != yuvFrame->width){

        if(NULL != m_pBufYuv420p)
        {
            free(m_pBufYuv420p);
            m_pBufYuv420p=NULL;
        }
    }

    m_nVideoW =  yuvFrame->width;
    m_nVideoH = yuvFrame->height;

    m_yFrameLength = yuvFrame->luma.length;
    m_uFrameLength = yuvFrame->chromaB.length;
    m_vFrameLength = yuvFrame->chromaR.length;


    //申请内存存一帧yuv图像数据,其大小为分辨率的1.5倍
    int nLen = m_yFrameLength + m_uFrameLength +m_vFrameLength;

    if(NULL == m_pBufYuv420p)
    {
        m_pBufYuv420p = ( unsigned char*) malloc(nLen);
    }

    memcpy(m_pBufYuv420p,yuvFrame->luma.dataBuffer,m_yFrameLength);
    memcpy(m_pBufYuv420p+m_yFrameLength,yuvFrame->chromaB.dataBuffer,m_uFrameLength);
    memcpy(m_pBufYuv420p+m_yFrameLength +m_uFrameLength,yuvFrame->chromaR.dataBuffer,m_vFrameLength);

    m_bUpdateData =true;

}
void CCGLVideoRender::ResizeGL(int w,int h)
{
    glViewport(0,0,w,h);
}

void CCGLVideoRender::SetupAssetManager(AAssetManager *pManager)
{
    m_pAssetManager = pManager;
}
void CCGLVideoRender::loadShaderResources(AAssetManager *pManager)
{
    m_pOpenGLShader->InitShadersFromFile(pManager,"yuvplayv.glsl","yuvplayf.glsl");
}
