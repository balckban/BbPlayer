cmake_minimum_required(VERSION 3.10)

SET(GLM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/3rdparty/libglm)
SET(FFMPEG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/3rdparty/libFFmpeg)
SET(CPPDIR ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp)

INCLUDE_DIRECTORIES(${GLM_DIR}/include)
INCLUDE_DIRECTORIES(${FFMPEG_DIR}/${ANDROID_ABI}/include)
LINK_DIRECTORIES(${FFMPEG_DIR}/${ANDROID_ABI}/lib)
SET(FFMPEG_LIB avformat avcodec avdevice avfilter avutil swresample swscale fdk-aac x264 z c m dl)

SET(LIB_SRC ${CPPDIR}/OpenSLInterface.c
        ${CPPDIR}/CCOpenGLShader.cpp
        ${CPPDIR}/CCGLVideoRender.cpp
        ${CPPDIR}/JCAudioPlayer.cpp
        ${CPPDIR}/JCAVCodecHandler.cpp
        ${CPPDIR}/JCMPlayer.cpp)

SET(ANDROID_SYSLIBS android log GLESv3 OpenSLES)

add_library(JCMPlayer SHARED  ${LIB_SRC})

target_link_libraries(JCMPlayer  ${FFMPEG_LIB} ${ANDROID_SYSLIBS})