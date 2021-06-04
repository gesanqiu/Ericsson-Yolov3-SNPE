#ifndef __TS_CAMERA_H__
#define __TS_CAMERA_H__

#include <gst/gst.h>
#include <gst/app/app.h>
#include <iostream>
#include <condition_variable>
#include <tr1/memory>
#include <unistd.h>
#include <sys/time.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>

#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/types_c.h"
#include <string.h>

#include "TsProAndCon.h"
#include  "YoloClassification.h"

using namespace cv;

class TsCamera;

/*-------------------------------------------------------------------------
    global class gstreamer pipline
-------------------------------------------------------------------------*/
class TsMulitGstCamPlayer
{
  public:
    static void GstEnvInit()
    {
        gst_init ( NULL, NULL );
    }
    static GstElement* GetPipeline()
    {
        return g_pPipeline;
    }
    static GstElement* GetDisplayPipeline()
    {
        return g_pOutPipeline;
    }
/**
 * [TsMulitGstCamPlayer.buildCamById
Create gstreamer pipline based on data stream]
 * @Author   zhaoyl
 * @param    nId  camera id
 * @param    strName  camera name
 * @param    nWidth  resolution width
 * @param    nHeight  resolution height
 * @param    strDecodeType  decode way h264/h265
 * @param    strUri  file path or rtsp
 * @return   TsCamera pipline
 */
    static TsCamera* buildCamById ( int nId, std::string strName, int nWidth,
                                int nHeight, std::string strDecodeType, std::string strUri, int nFramerate );
    static void GstEnvDeinit()
    {
        if ( g_pPipeline!=NULL )
        {
            gst_element_set_state ( g_pPipeline, GST_STATE_NULL );
            gst_object_unref ( g_pPipeline );
            g_pPipeline = NULL;
        }
        if ( g_pOutPipeline!=NULL )
        {
            gst_element_set_state ( g_pOutPipeline, GST_STATE_NULL );
            gst_object_unref ( g_pOutPipeline );
            g_pOutPipeline = NULL;
        }
    }

  private:
    static GstElement* g_pOutPipeline;
    static GstElement* g_pPipeline;
};

class TsCamera
{
  public:
    TsCamera();
    ~TsCamera();
/**
 * [TsCamera.BuildPipeLine create gstreamer pipeline ]
 * @Author   zhaoyl
 * @param    rtsp  if it is rtsp stream, true, otherwise false
 * @param    isHwDec if it sets hardward decode, select true
 * @return   void
 */
    void BuildPipeLine ( bool rtsp, bool isHwDec );
/**
 * [TsCamera.Init init env pipeline]
 * @Author   zhaoyl
 * @param    pipeline  instance pipeline by TsMulitGstCamPlayer::GetPipeline
 * @return                            [description]
 */
    void Init (  );
/**
 * [TsCamera.Deinit deinit gst pipeline]
 * @Author   zhaoyl
 * @return   void
 */
    void Deinit();
/**
 * [TsCamera.SetName set camera pipeline name]
 * @Author   zhaoyl
 * @param    name  set camera name
 * @return   void
 */
    void SetName ( std::string name );
    std::string GetName();
    std::string GetAppSrcName();
    std::string GetWaylandName();
/**
 * [TsCamera.SetWidth set camera video resolution width]
 * @Author   zhaoyl
 * @param    width  vidoe stream resolution width
 * @return   void
 */
    void SetWidth ( int width );
    int GetWidth();
/**
 * [TsCamera.SetHeight set camera video resolution width]
 * @Author   zhaoyl
 * @param    height  vidoe stream resolution height
 * @return   void
 */
    void SetHeight ( int height );
    int GetHeight();
/**
 * [TsCamera.SetDecodeType set camera vidoe decode way h264/h265]
 * @Author   zhaoyl
 * @param    type  decode way h264/h265
 * @return   void
 */
    void SetDecodeType ( std::string type );
    std::string GetDecodeType();
/**
 * [TsCamera.SetUri set camera vidoe stream uri path]
 * @Author   zhaoyl
 * @param    uri  uri path, eg. rtsp:// or /home/../test.mp4
 * @return   void
 */
    void SetUri ( std::string uri );
/**
 * [TsCamera.isHwDec whether if decode way is hardway decode]
 * @Author   zhaoyl
 * @return   void
 */
    bool isHwDec()
    {
        return m_bHwDec;
    }
    std::string GetUri();
/**
 * [TsCamera.SetCameraID description]
 * @Author   zhaoyl
 * @param    id  set camera flag id
 * @return   void
 */
    void SetCameraID ( int id );
    int GetCameraID();
/**
 * [TsCamera.SetEnable enable pipeline camera]
 * @Author   zhaoyl
 * @param    state  if set pipeline is enable, true
 * @return   void
 */
    void SetEnable ( bool state );
/**
 * [TsCamera.SetFramerate set camera decode framerate]
 * @Author   zhaoyl
 * @param    rate  framerate eg. 20fps
 * @return   void
 */
    void SetFramerate ( int rate );
    int GetFramerate();
/**
 * [TsCamera.CatureNoWait Get data gstsample after decoded]
 * @Author   zhaoyl
 * @param    dst  return point object gstsample
 * @return   void
 */
    void CatureConWait ( std::shared_ptr<GstSample>& dst );
/**
 * [TsCamera.addObjectInfo set face info to queue]
 * @Author   zhaoyl
 * @param    faceinfo  object faceinfo to queue
 * @return   void
 */
    void addObjectInfo ( S_OBJECT_DATA faceInfo );
/**
 * [TsCamera.clearFaceList clear list]
 * @Author   zhaoyl
 * @return   void
 */
    void clearObjectList();
/**
 * [TsCamera.getObjectInfoList Get faceinfo list]
 * @Author   zhaoyl
 * @return   get faceinfo list
 */
    std::deque<S_OBJECT_DATA> getObjectInfoList();

    bool m_bPrintStreamInfo;

    TsProAndCon<cv::Mat> frame_cache_1;
    TsProAndCon<cv::Mat> frame_cache_2;
    TsProAndCon<cv::Mat> frame_cache_3;
    TsProAndCon<cv::Mat> frame_cache_4;
    TsProAndCon<cv::Mat> frame_cache_5;
    TsProAndCon<cv::Mat> frame_cache_6;
    TsProAndCon<cv::Mat> show_frame_cache_1;
    TsProAndCon<cv::Mat> show_frame_cache_2;
    TsProAndCon<cv::Mat> show_frame_cache_3;
    TsProAndCon<cv::Mat> show_frame_cache_4;
    TsProAndCon<cv::Mat> show_frame_cache_5;
    TsProAndCon<cv::Mat> show_frame_cache_6;

    static cv::Mat unkownCamMat;

    int w_show_x;
    int w_show_y;
    int w_show_width;
    int w_show_height;

  private:
    std::string m_strline;
    std::string m_stroutline;
    bool m_bInited;
    bool m_bEnable;
/**
 * [TsCamera.onEOS appsink callback oneos event]
 * @Author   zhaoyl
 * @param    appsink  object appsink by sink name
 * @param    user_data  customized user data
 * @return   void
 */
    static void onEOS ( GstAppSink* appsink, void* user_data );
/**
 * [TsCamera.onPreroll appsink callback onpreroll event]
 * @Author   zhaoyl
 * @param    appsink  object appsink by sink name
 * @param    user_data  customized user data
 * @return   void
 */
    static GstFlowReturn onPreroll ( GstAppSink* appsink, void* user_data );
/**
 * [TsCamera.onBuffer appsink callback onbuffer event]
 * @Author   zhaoyl
 * @param    appsink  object appsink by sink name
 * @param    user_data  customized user data
 * @return   void
 */
    static GstFlowReturn onBuffer ( GstAppSink* appsink, void* user_data );

    static void cbNeedData (GstElement *appsrc,
                      guint       unused_size,
                      gpointer    user_data);

/**
 * [TsCamera.MY_BUS_CALLBACK recieve message from bus event]
 * @Author   zhaoyl
 * @param    bus  init bus element
 * @param    message  init message elemnet
 * @param    user_data  customized user data
 * @return   if success return true
 */
    static gboolean MY_BUS_CALLBACK ( GstBus* bus, GstMessage* message, gpointer data );

    GstElement* m_pPipeline;
    GstElement* m_pSink;

    GstElement* m_pOutPipeline;
    GstElement* m_pAppSrcSink;
    GstBus* m_pBus;
    GstBus* m_pOutBus;

    std::string m_strName;
    std::string m_strAppsrcName;
    std::string m_strWaylandName;
    int m_nWidth;
    int m_nHeight;
    std::string m_strDecodeType;
    std::string m_strUri;
    int m_nId;
    int m_nFramerate;
    bool m_bHwDec;

    std::mutex m_ObjectListMut;
    std::deque<S_OBJECT_DATA> m_ObjectInfoQueue;

};


#endif
