#include "TsCamera.h"
#include <math.h>
#include <glib.h>
#include <stdlib.h>
#include <thread>
#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include <chrono>
#include <time.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <opencv2/imgcodecs/legacy/constants_c.h>
#include "YoloClassification.h"


#define OBJECT_DETECT_MAX   2

#define AI_RES_WIDTH 1920
#define AI_RES_HEIGHT 1080

#define GST_RTSPSRC_PATH   "GST_RTSPSRC_PATH"

enum queueCtrl{
    CACHE_1 = 1,
    CACHE_2,
    CACHE_3,
    CACHE_4,
    CACHE_5,
    CACHE_6,
};

static float same_threshold = 0.7;
std::string* labels = NULL;
YoloClassification* DSP_Detector_1 = NULL;
YoloClassification* DSP_Detector_2 = NULL;
YoloClassification* DSP_Detector_3 = NULL;
YoloClassification* DSP_Detector_4 = NULL;
YoloClassification* DSP_Detector_5 = NULL;
YoloClassification* DSP_Detector_6 = NULL;

GstElement* TsMulitGstCamPlayer::g_pPipeline = NULL;
GstElement* TsMulitGstCamPlayer::g_pOutPipeline = NULL;

TsProAndCon<cv::Mat> rgb_frame_cache;
std::vector<TsCamera*> _listTsCam;
enum queueCtrl appsinkQueueCtrl = CACHE_1;
enum queueCtrl appsrcQueueCtrl = CACHE_1;

TsCamera* TsMulitGstCamPlayer::buildCamById ( int nId, std::string strName, int nWidth,
                                              int nHeight, std::string strDecodeType, std::string strUri, int nFramerate )
{
    return NULL;
}

cv::Mat TsCamera::unkownCamMat;

TsCamera::TsCamera()
{
    m_bInited = false;
    m_bEnable = false;
    m_bPrintStreamInfo = false;
    m_strName = "unkown";
}
TsCamera::~TsCamera()
{
    m_bInited = false;
    m_bEnable = false;
    m_bPrintStreamInfo = false;
}
void TsCamera::Init ( )
{
    GError* error = NULL;

    if ( !m_bEnable || !m_bInited )
    {
        printf ( "[Camera %s]not enable or init str pipeline\n", GetName().c_str() );
        goto exit;
    }

    /* create a new pipeline */
    m_pPipeline = gst_parse_launch ( m_strline.c_str(), &error );

    if ( error != NULL )
    {
        printf ( "[Camera %s]could not construct pipeline: %s\n", GetName().c_str(), error->message);
        g_clear_error ( &error );
        goto exit;
    }

    /* get sink */
    m_pSink = gst_bin_get_by_name ( GST_BIN ( m_pPipeline ), GetName().c_str() );

    gst_app_sink_set_emit_signals ( ( GstAppSink* ) m_pSink, true );
    gst_app_sink_set_drop ( ( GstAppSink* ) m_pSink, true );
    gst_app_sink_set_max_buffers ( ( GstAppSink* ) m_pSink, 1 );
    gst_base_sink_set_last_sample_enabled ( GST_BASE_SINK ( m_pSink ), true );
    gst_base_sink_set_max_lateness ( GST_BASE_SINK ( m_pSink ), 0 );
    {
        //avoid goto check
        GstAppSinkCallbacks callbacks = { onEOS, onPreroll, onBuffer };
        gst_app_sink_set_callbacks ( GST_APP_SINK ( m_pSink ), &callbacks, reinterpret_cast<void*> ( this ), NULL );
    }

    /* Putting a Message handler */
    m_pBus = gst_pipeline_get_bus ( GST_PIPELINE ( m_pPipeline ) );
    gst_bus_add_watch ( m_pBus, MY_BUS_CALLBACK, reinterpret_cast<void*> ( this ) );
    gst_object_unref ( m_pBus );

    /* Run the pipeline */
    printf ( "[Camera %s]Playing: %s\n", GetName().c_str(), GetUri().c_str() );
    gst_element_set_state ( m_pPipeline, GST_STATE_PLAYING );

#if 1
    /* create a Display pipeline */
    m_pOutPipeline = gst_parse_launch ( m_stroutline.c_str(), &error );

    if ( error != NULL )
    {
        printf ( "[Camera %s]could not construct pipeline: %s\n", GetName().c_str(), error->message);
        g_clear_error ( &error );
        goto exit;
    }

    /* get sink */
    m_pAppSrcSink = gst_bin_get_by_name ( GST_BIN ( m_pOutPipeline ), GetAppSrcName().c_str() );

    /* Putting a Message handler */
    m_pOutBus = gst_pipeline_get_bus ( GST_PIPELINE ( m_pOutPipeline ) );
    gst_bus_add_watch ( m_pOutBus, MY_BUS_CALLBACK, reinterpret_cast<void*> ( this ) );
    gst_object_unref ( m_pOutBus );

    /* appsrc sink */
    g_signal_connect (m_pAppSrcSink, "need-data", G_CALLBACK (cbNeedData), reinterpret_cast<void*> ( this ));


    /* Run the pipeline */
    printf ( "[Camera %s]Display: %s\n", GetName().c_str(), GetUri().c_str() );

    gst_element_set_state ( m_pOutPipeline, GST_STATE_PLAYING );
#endif
    return;
exit:
    if ( m_pSink!=NULL )
    {
        gst_object_unref ( m_pSink );
        m_pSink = NULL;
    }
    if ( m_pPipeline!=NULL )
    {
        gst_element_set_state ( m_pPipeline, GST_STATE_NULL );
        gst_object_unref ( m_pPipeline );
        m_pPipeline = NULL;
    }
    if ( m_pAppSrcSink!=NULL )
    {
        gst_object_unref ( m_pAppSrcSink );
        m_pAppSrcSink = NULL;
    }
    if ( m_pOutPipeline!=NULL )
    {
        gst_element_set_state ( m_pOutPipeline, GST_STATE_NULL );
        gst_object_unref ( m_pOutPipeline );
        m_pOutPipeline = NULL;
    }
    m_bEnable = false;
    m_bInited = false;
}
void TsCamera::Deinit()
{
    m_bEnable = false;
    m_bInited = false;
    if ( m_pSink!=NULL )
    {
        gst_object_unref ( m_pSink );
        m_pSink = NULL;
    }
    if ( m_pPipeline!=NULL )
    {
        gst_element_set_state ( m_pPipeline, GST_STATE_NULL );
        gst_object_unref ( m_pPipeline );
        m_pPipeline = NULL;
    }
    if ( m_pAppSrcSink!=NULL )
    {
        gst_object_unref ( m_pAppSrcSink );
        m_pAppSrcSink = NULL;
    }
    if ( m_pOutPipeline!=NULL )
    {
        gst_element_set_state ( m_pOutPipeline, GST_STATE_NULL );
        gst_object_unref ( m_pOutPipeline );
        m_pOutPipeline = NULL;
    }

    printf ( "[Camera %s]End of the Streaming... ending the playback\n", GetName().c_str() );
    printf ( "[Camera %s]Eliminating Pipeline\n", GetName().c_str() );
}
// onEOS
void TsCamera::onEOS ( GstAppSink* appsink, void* user_data )
{
    TsCamera* dec = reinterpret_cast<TsCamera*> ( user_data );
    printf ( "[Camera %s]gstreamer decoder onEOS\n", dec->GetName().c_str() );
}

// onPreroll
GstFlowReturn TsCamera::onPreroll ( GstAppSink* appsink, void* user_data )
{
    TsCamera* dec = reinterpret_cast<TsCamera*> ( user_data );
    printf ( "[Camera %s]gstreamer decoder onPreroll\n", dec->GetName().c_str() );
    return GST_FLOW_OK;
}


void deleterGstSample ( GstSample* x )
{
    //std::cout << "DELETER FUNCTION CALLED\n";
    if ( x != NULL )
    {
        gst_sample_unref ( x );
    }
}

// onBuffer
GstFlowReturn TsCamera::onBuffer ( GstAppSink* appsink, void* user_data )
{
    TsCamera* dec = NULL;
    GstSample* sample = NULL;
    dec = reinterpret_cast<TsCamera*> ( user_data );
    const GstStructure* info = NULL;
    GstBuffer* buffer = NULL;
    GstMapInfo map;
    GstCaps* caps = NULL;
    int sample_width = 0;
    int sample_height = 0;

    if ( dec == NULL || appsink == NULL )
    {
        printf ( "[Camera %s]decode or appsink is null\n", dec->GetName().c_str() );
        goto ONBUFFER_ERR;
    }

    sample = gst_base_sink_get_last_sample ( GST_BASE_SINK ( appsink ) );

    if ( sample == NULL )
    {
        printf ( "[Camera %s]pull sample is null\n", dec->GetName().c_str() );
    }
    else
    {
        buffer = gst_sample_get_buffer ( sample );
        if ( buffer == NULL ) {
            printf ( "[Camera %s]get buffer is null\n", dec->GetName().c_str() );
            goto ONBUFFER_ERR;
        }

        gst_buffer_map ( buffer, &map, GST_MAP_READ );

        caps = gst_sample_get_caps ( sample );
        if ( caps == NULL ) {
            printf ( "[Camera %s]get caps is null\n", dec->GetName().c_str() );
            goto ONBUFFER_ERR;
        }

        info = gst_caps_get_structure ( caps, 0 );
        if ( info == NULL ) {
            printf ( "[Camera %s]get info is null\n", dec->GetName().c_str() );
            goto ONBUFFER_ERR;
        }

        // ---- Read frame and convert to opencv format ---------------
        // convert gstreamer data to OpenCV Mat, you could actually
        // resolve height / width from caps...
        gst_structure_get_int ( info, "width", &sample_width );
        gst_structure_get_int ( info, "height", &sample_height );

        // appsink product queue produce
        // switch control push into available queue
        {
            cv::Mat tmpmat ( sample_height, sample_width, CV_8UC3, ( unsigned char* )map.data, cv::Mat::AUTO_STEP );
            tmpmat = tmpmat.clone();

            switch (appsinkQueueCtrl)
            {
            case CACHE_1:
                dec->frame_cache_1.product(std::make_shared<cv::Mat> (tmpmat));
                printf("[%s in %d]appsink push in queue one, queue size: %d\n", __FUNCTION__, __LINE__, dec->frame_cache_1.getCurrentSize());
                appsinkQueueCtrl = CACHE_2;
                break;
            case CACHE_2:
                dec->frame_cache_2.product(std::make_shared<cv::Mat> (tmpmat));
                printf("[%s in %d]appsink push in queue two, queue size: %d\n", __FUNCTION__, __LINE__, dec->frame_cache_2.getCurrentSize());
                appsinkQueueCtrl = CACHE_3;
                break;
            case CACHE_3:
                dec->frame_cache_3.product(std::make_shared<cv::Mat> (tmpmat));
                printf("[%s in %d]appsink push in queue three, queue size: %d\n", __FUNCTION__, __LINE__, dec->frame_cache_3.getCurrentSize());
                appsinkQueueCtrl = CACHE_4;
                break;
            case CACHE_4:
                dec->frame_cache_4.product(std::make_shared<cv::Mat> (tmpmat));
                printf("[%s in %d]appsink push in queue four, queue size: %d\n", __FUNCTION__, __LINE__, dec->frame_cache_4.getCurrentSize());
                appsinkQueueCtrl = CACHE_5;
            case CACHE_5:
                dec->frame_cache_5.product(std::make_shared<cv::Mat> (tmpmat));
                printf("[%s in %d]appsink push in queue five, queue size: %d\n", __FUNCTION__, __LINE__, dec->frame_cache_5.getCurrentSize());
                appsinkQueueCtrl = CACHE_6;
                break;
            case CACHE_6:
                dec->frame_cache_6.product(std::make_shared<cv::Mat> (tmpmat));
                printf("[%s in %d]appsink push in queue six, queue size: %d\n", __FUNCTION__, __LINE__, dec->frame_cache_6.getCurrentSize());
                appsinkQueueCtrl = CACHE_1;
                break;
            default:
                break;
            } 
        }
    }

ONBUFFER_ERR:
    if (buffer) {
        gst_buffer_unmap ( buffer, &map );
    }
    if (sample) {
        gst_sample_unref (sample);
    }
    return GST_FLOW_OK;
}

void TsCamera::BuildPipeLine ( bool rtsp, bool isHwDec )
{
    std::ostringstream cameraPath;
    std::ostringstream cameraOutPath;
    int width = AI_RES_WIDTH;
    int height = AI_RES_HEIGHT;
    cameraPath << "rtspsrc location=" << GetUri() << " latency=500 tcp-timeout=500 drop-on-latency=true ntp-sync=true" << " ! ";
    cameraPath << "queue ! rtp" << GetDecodeType() << "depay ! "<< GetDecodeType() << "parse ! queue ! qtivdec " << " ! ";
    cameraPath << "queue ! qtivtransform ! video/x-raw,format=BGR,width=" << width << ",height=" << height;
    cameraPath << " ! appsink name=" << GetName() << " sync=false ";
    cameraPath << "caps=video/x-raw,format=BGR,width=" << width << ",height=" << height << " ";

    cameraOutPath << "appsrc name=" << GetAppSrcName() << " stream-type=0 format=3 is-live=true ";
    cameraOutPath << " caps=video/x-raw,format=BGR,width=" << width << ",height=" << height << " ! ";
    
    // waylandsink pipeline
    // cameraOutPath << "videoconvert ! waylandsink name=" << GetWaylandName() << " sync=false x=" << w_show_x << " y=" << w_show_y << " width=" << w_show_width << " height=" << w_show_height;
    
    // rtmpsink pipeline
    cameraOutPath << "videoconvert ! video/x-raw,width=1920,height=1080,format=NV12 ! queue ! omxh264enc target-bitrate=4000000 interval-intraframes=25 quant-b-frames=0 control-rate=variable ! queue ! h264parse ! queue ! flvmux ! queue ! rtmpsink max-lateness=500000000 sync=false location=rtmp://52.81.79.48:1935/live/mask/0";

    m_strline = cameraPath.str();
    m_stroutline = cameraOutPath.str();
    printf ( "[Camera %s]gstreamer decoder pipeline string:%s\n", GetName().c_str(), m_strline.c_str() );
    printf ( "[Camera %s]gstreamer display pipeline string:%s\n", GetName().c_str(), m_stroutline.c_str() );
    m_bInited = true;
    m_bHwDec = isHwDec;
}
void TsCamera::SetName ( std::string name )
{
    m_strName = name;
    m_strAppsrcName = m_strName + "_app";
    m_strWaylandName = m_strName + "_wayland";
}
std::string TsCamera::GetName()
{
    return m_strName;
}
std::string TsCamera::GetAppSrcName()
{
    return m_strAppsrcName;
}
std::string TsCamera::GetWaylandName()
{
    return m_strWaylandName;
}
void TsCamera::SetWidth ( int width )
{
    m_nWidth = width;
}
int TsCamera::GetWidth()
{
    return m_nWidth;
}
void TsCamera::SetHeight ( int height )
{
    m_nHeight = height;
}
int TsCamera::GetHeight()
{
    return m_nHeight;
}
void TsCamera::SetDecodeType ( std::string type )
{
    m_strDecodeType = type;
}
std::string TsCamera::GetDecodeType()
{
    return m_strDecodeType;
}
void TsCamera::SetUri ( std::string uri )
{
    m_strUri = uri;
}
std::string TsCamera::GetUri()
{
    std::cout << cv::getBuildInformation() << std::endl;
    return m_strUri;//"\"rtsp://10.0.20.158:554/user=admin&password=&channel=1&stream=0.sdp?\"";
}
void TsCamera::SetCameraID ( int id )
{
    m_nId = id;
}
int TsCamera::GetCameraID()
{
    return m_nId;
}
void TsCamera::SetEnable ( bool state )
{
    m_bEnable = state;
}
void TsCamera::SetFramerate ( int rate )
{
    m_nFramerate = rate;
}
int TsCamera::GetFramerate()
{
    return m_nFramerate;//25;
}

void TsCamera::CatureConWait ( std::shared_ptr<GstSample>& dst )
{
    if ( !m_bEnable || !m_bInited )
    {
        printf ( "[Camera %s]not enable or init str pipeline\n", GetName().c_str() );
        return;
    }

    // frame_cache.consumption(dst);

}

void TsCamera::addObjectInfo ( S_OBJECT_DATA objectInfo )
{
    std::unique_lock<std::mutex> lk ( m_ObjectListMut );
    m_ObjectInfoQueue.push_back ( objectInfo );
}

void TsCamera::clearObjectList()
{
    std::unique_lock<std::mutex> lk ( m_ObjectListMut );
    m_ObjectInfoQueue.clear();
}

std::deque<S_OBJECT_DATA> TsCamera::getObjectInfoList()
{
    std::unique_lock<std::mutex> lk ( m_ObjectListMut );
    std::deque<S_OBJECT_DATA> list = m_ObjectInfoQueue;
    return list;
}

std::string GetLocalDateTime ( void )
{
    std::string defaultTime = "1970-01-01 00:00:00:000";
    try {
        struct timeval curTime;
        gettimeofday ( &curTime, NULL );

        char buffer[80] = {0};
        struct tm nowTime;
        localtime_r ( &curTime.tv_sec, &nowTime ); //把得到的值存入临时分配的内存中，线程安全
        strftime ( buffer, sizeof ( buffer ), "%Y-%m-%d", &nowTime );

        char currentTime[84] = {0};
        snprintf ( currentTime, sizeof ( currentTime ), "%s", buffer);

        return currentTime;
    } catch ( const std::exception& e ) {
        return defaultTime;
    } catch ( ... ) {
        return defaultTime;
    }
}

void TsCamera::cbNeedData (GstElement *appsrc,
                  guint       unused_size,
                  gpointer    user_data)
{
    TsCamera* dec = reinterpret_cast<TsCamera*> ( user_data );
    static GstClockTime timestamp = 0;
    GstBuffer *buffer;
    GstFlowReturn ret;

    std::shared_ptr<cv::Mat> imgframe;

    {
        // appsrc consumer queue consume
        // pop frame from appsrcQueueCtrl
        switch (appsrcQueueCtrl)
        {
        case CACHE_1:
            dec->show_frame_cache_1.consumption(imgframe);
            printf("[%s in %d]appsrc pop from queue one, queue size: %d\n", __FUNCTION__, __LINE__, dec->show_frame_cache_1.getCurrentSize());
            appsrcQueueCtrl = CACHE_2;
            break;
        case CACHE_2:
            dec->show_frame_cache_2.consumption(imgframe);
            printf("[%s in %d]appsrc pop from queue two, queue size: %d\n", __FUNCTION__, __LINE__, dec->show_frame_cache_2.getCurrentSize());
            appsrcQueueCtrl = CACHE_3;
            break;
        case CACHE_3:
            dec->show_frame_cache_3.consumption(imgframe);
            printf("[%s in %d]appsrc pop from queue three, queue size: %d\n", __FUNCTION__, __LINE__, dec->show_frame_cache_3.getCurrentSize());
            appsrcQueueCtrl = CACHE_4;
            break;
        case CACHE_4:
            dec->show_frame_cache_4.consumption(imgframe);
            printf("[%s in %d]appsrc pop from queue four, queue size: %d\n", __FUNCTION__, __LINE__, dec->show_frame_cache_4.getCurrentSize());
            appsrcQueueCtrl = CACHE_5;
            break;
        case CACHE_5:
            dec->show_frame_cache_5.consumption(imgframe);
            printf("[%s in %d]appsrc pop from queue five, queue size: %d\n", __FUNCTION__, __LINE__, dec->show_frame_cache_5.getCurrentSize());
            appsrcQueueCtrl = CACHE_6;
            break;
        case CACHE_6:
            dec->show_frame_cache_6.consumption(imgframe);
            printf("[%s in %d]appsrc pop from queue six, queue size: %d\n", __FUNCTION__, __LINE__, dec->show_frame_cache_6.getCurrentSize());
            appsrcQueueCtrl = CACHE_1;
        default:
            break;
        }
    }

    if ( imgframe == NULL || imgframe.get() == NULL )
    {
        imgframe = std::make_shared<cv::Mat>(AI_RES_HEIGHT, AI_RES_WIDTH, CV_8UC3, cv::Scalar(0, 0, 0));
    }

    int len = imgframe->total() * imgframe->elemSize();
    buffer = gst_buffer_new_allocate (NULL, len, NULL);

    /* this makes the image */
    GstMapInfo map;
    gst_buffer_map(buffer,&map,GST_MAP_WRITE);
    memcpy(map.data,imgframe->data, len);   //ptr->input 即一个mat指针
    GST_BUFFER_PTS (buffer) = timestamp;
    GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, 25);
    timestamp += GST_BUFFER_DURATION (buffer) ;
    g_signal_emit_by_name (appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unmap(buffer,&map);
    gst_buffer_unref (buffer);

    if (ret != GST_FLOW_OK) {
      /* something wrong, stop pushing */
      printf ( "[Camera %s]Error: push-buffer fail\n", dec->GetName().c_str());
    }
    // usleep (20 * 1000);
}

gboolean TsCamera::MY_BUS_CALLBACK ( GstBus* bus, GstMessage* message, gpointer data )
{
    TsCamera* dec = reinterpret_cast<TsCamera*> ( data );
    //printf ("[Camera %s]Got %s message\n", dec->GetName().c_str(), GST_MESSAGE_TYPE_NAME (message));
    switch ( GST_MESSAGE_TYPE ( message ) )
    {
        case GST_MESSAGE_ERROR:
        {
            GError* err;
            gchar* debug;

            gst_message_parse_error ( message, &err, &debug );
            printf ( "[Camera %s]Error: %s\n", dec->GetName().c_str(), err->message );
            g_error_free ( err );
            g_free ( debug );
            exit(0);
            break;
        }
        case GST_MESSAGE_EOS:
            /* end-of-stream */
            exit(0);
            break;
        default:
            /* unhandled message */
            break;
    }
    /* we want to be notified again the next time there is a message
    * on the bus, so returning TRUE (FALSE means we want to stop watching
    * for messages on the bus and our callback should not be called again)
    */
    return TRUE;
}

/**
* @brief draw rectangles of detected objects
* @param shared_ptr<cv::Mat> imgframe
* @param std::deque<S_OBJECT_DATA> detectedlist
* @return {void}
* @Author Ricardo Lu
*/
void drawObjectRect(std::shared_ptr<cv::Mat> &img, std::vector<S_OBJECT_DATA> detectedlist)
{
    int left = 0;
    int top = 0;
    std::vector<S_OBJECT_DATA> objectlist =  detectedlist;
    unsigned int size = objectlist.size();

    for (unsigned int i = 0; i < size; i++ )
    {
        S_OBJECT_DATA data = objectlist[i];
        if (data.score >= same_threshold)
        {
            cv::Scalar color(150, 255, 40);
            std::string words= " " + labels[data.label];
            std::cout << "[" << __FUNCTION__ << " in " << __LINE__ << "]" << "yolov3 detect output: "  <<  words <<std::endl;
            cv::Point frontpos= cv::Point ( ( int ) ( data.x+left ), ( int ) ( data.y+top - 10 ) );
            cv::putText(*img, words, frontpos, cv::FONT_HERSHEY_COMPLEX, 0.8, color,2,0.3);
        }
        // merge face box;
        //LOGD("fd(%d) x=%d, y=%d , width=%d, height=%d", k, (int)(data.x), (int)(data.y), (int)(data.width), (int)(data.height));
        cv::rectangle(*img, cv::Rect((int)(data.x+left), (int)(data.y+top), (int)data.width, (int)data.height), cv::Scalar(0, 200, 0), 3);
        int grapsize = 15;
        int thickness=2;
        cv::Scalar scalar ( 0, 200, 0 );
        cv::line ( *img, cv::Point ( ( int ) ( data.x+left ), ( int ) ( data.y+top ) ), cv::Point ( ( int ) ( data.x+grapsize+left ), ( int ) ( data.y+top ) ), scalar, thickness );
        cv::line ( *img, cv::Point ( ( int ) ( data.x+left ), ( int ) ( data.y+top ) ), cv::Point ( ( int ) ( data.x+left ), ( int ) ( data.y+grapsize+top ) ), scalar, thickness );
        cv::line ( *img, cv::Point ( ( int ) ( data.x+left + data.width - grapsize ), ( int ) ( data.y+top ) ), cv::Point ( ( int ) ( data.x+left + data.width ), ( int ) ( data.y+top ) ), scalar, thickness );
        cv::line ( *img, cv::Point ( ( int ) ( data.x+left + data.width ), ( int ) ( data.y+top ) ), cv::Point ( ( int ) ( data.x+left + data.width ), ( int ) ( data.y+grapsize+top ) ), scalar, thickness );
        cv::line ( *img, cv::Point ( ( int ) ( data.x+left ), ( int ) ( data.y+data.height-grapsize+top ) ), cv::Point ( ( int ) ( data.x+left ), ( int ) ( data.y+data.height+top ) ), scalar, thickness );
        cv::line ( *img, cv::Point ( ( int ) ( data.x+left ), ( int ) ( data.y+data.height+top ) ), cv::Point ( ( int ) ( data.x+grapsize+left ), ( int ) ( data.y+data.height+top ) ), scalar, thickness );
        cv::line ( *img, cv::Point ( ( int ) ( data.x+left-grapsize+ data.width ), ( int ) ( data.y+data.height+top ) ), cv::Point ( ( int ) ( data.x+left+ data.width ), ( int ) ( data.y+data.height+top ) ), scalar, thickness );
        cv::line ( *img, cv::Point ( ( int ) ( data.x+left+ data.width ), ( int ) ( data.y+data.height+top ) ), cv::Point ( ( int ) ( data.x+left+ data.width ), ( int ) ( data.y-grapsize+data.height+top ) ), scalar, thickness );
    }
}

void thread_do_detect_1(void)
{
    while(true)
    {
        TsCamera* pCam = _listTsCam.at ( 0 );
        if ( pCam->GetName().compare ( "unkown" ) == 0 )
        {
            continue;
        }

        std::shared_ptr<cv::Mat> imgframe;
        std::vector<S_OBJECT_DATA> vec_object_rect;

        pCam->frame_cache_1.consumption(imgframe);
	    vec_object_rect = yolov3_detection(imgframe, DSP_Detector_1);
        std::cout << "[" << __FUNCTION__ << " in " << __LINE__ << "]" << "yolov3 detect output size: "  <<  vec_object_rect.size() <<std::endl;

        drawObjectRect(imgframe, vec_object_rect);
        pCam->show_frame_cache_1.product(imgframe);
    }
}

void thread_do_detect_2(void)
{
    while (true) {
        TsCamera* pCam = _listTsCam.at ( 0 );
        if ( pCam->GetName().compare ( "unkown" ) == 0 )
        {
            continue;
        }

        std::shared_ptr<cv::Mat> imgframe;
        std::vector<S_OBJECT_DATA> vec_object_rect;

        pCam->frame_cache_2.consumption(imgframe);
	    vec_object_rect = yolov3_detection(imgframe, DSP_Detector_2);
        std::cout << "[" << __FUNCTION__ << " in " << __LINE__ << "]" << "yolov3 detect output size: "  <<  vec_object_rect.size() <<std::endl;

        drawObjectRect(imgframe, vec_object_rect);
        pCam->show_frame_cache_2.product(imgframe);
    }  
}

void thread_do_detect_3(void)
{
    while (true) {
        TsCamera* pCam = _listTsCam.at ( 0 );
        if ( pCam->GetName().compare ( "unkown" ) == 0 )
        {
            continue;
        }

        std::shared_ptr<cv::Mat> imgframe;
        std::vector<S_OBJECT_DATA> vec_object_rect;

        pCam->frame_cache_3.consumption(imgframe);
	    vec_object_rect = yolov3_detection(imgframe, DSP_Detector_3);
        std::cout << "[" << __FUNCTION__ << " in " << __LINE__ << "]" << "yolov3 detect output size: "  <<  vec_object_rect.size() <<std::endl;

        drawObjectRect(imgframe, vec_object_rect);
        pCam->show_frame_cache_3.product(imgframe);
    }  
}

void thread_do_detect_4(void)
{
    while (true) {
        TsCamera* pCam = _listTsCam.at ( 0 );
        if ( pCam->GetName().compare ( "unkown" ) == 0 )
        {
            continue;
        }

        std::shared_ptr<cv::Mat> imgframe;
        std::vector<S_OBJECT_DATA> vec_object_rect;

        pCam->frame_cache_4.consumption(imgframe);
	    vec_object_rect = yolov3_detection(imgframe, DSP_Detector_4);
        std::cout << "[" << __FUNCTION__ << " in " << __LINE__ << "]" << "yolov3 detect output size: "  <<  vec_object_rect.size() <<std::endl;

        drawObjectRect(imgframe, vec_object_rect);
        pCam->show_frame_cache_4.product(imgframe);
    }  
}

void thread_do_detect_5(void)
{
    while (true) {
        TsCamera* pCam = _listTsCam.at ( 0 );
        if ( pCam->GetName().compare ( "unkown" ) == 0 )
        {
            continue;
        }

        std::shared_ptr<cv::Mat> imgframe;
        std::vector<S_OBJECT_DATA> vec_object_rect;

        pCam->frame_cache_5.consumption(imgframe);
	    vec_object_rect = yolov3_detection(imgframe, DSP_Detector_5);
        std::cout << "[" << __FUNCTION__ << " in " << __LINE__ << "]" << "yolov3 detect output size: "  <<  vec_object_rect.size() <<std::endl;

        drawObjectRect(imgframe, vec_object_rect);
        pCam->show_frame_cache_5.product(imgframe);
    }  
}

void thread_do_detect_6(void)
{
    while (true) {
        TsCamera* pCam = _listTsCam.at ( 0 );
        if ( pCam->GetName().compare ( "unkown" ) == 0 )
        {
            continue;
        }

        std::shared_ptr<cv::Mat> imgframe;
        std::vector<S_OBJECT_DATA> vec_object_rect;

        pCam->frame_cache_6.consumption(imgframe);
	    vec_object_rect = yolov3_detection(imgframe, DSP_Detector_6);
        std::cout << "[" << __FUNCTION__ << " in " << __LINE__ << "]" << "yolov3 detect output size: "  <<  vec_object_rect.size() <<std::endl;

        drawObjectRect(imgframe, vec_object_rect);
        pCam->show_frame_cache_6.product(imgframe);
    }  
}

/**
 * [start_task start task init process]
 * @Author   zhaoyl
 * @param    void
 */
void start_task ( void )
{
    TsMulitGstCamPlayer::GstEnvInit();

    char* sourcepath;
    if (!(sourcepath = getenv(GST_RTSPSRC_PATH))) {
        printf("[%s in %d] error getenv, exit program.\n", __FUNCTION__, __LINE__);
        return ;
    }
    printf("GST_RTSPSRC_PATH= %s\n", sourcepath);

    TsCamera* pCam = new TsCamera();

    pCam->SetName ( "CameraOne" );
    pCam->w_show_x = 0;
    pCam->w_show_y = 0;
    pCam->w_show_width = 1920;
    pCam->w_show_height = 1080;
    pCam->SetWidth ( 1920 );
    pCam->SetHeight ( 1080 );
    pCam->SetDecodeType ( "h264" );
    pCam->SetCameraID ( 0 );
    pCam->SetFramerate ( 20 );
    pCam->SetUri (std::string(sourcepath));
    pCam->BuildPipeLine ( true, true );

    pCam->SetEnable ( true );
    pCam->Init ( );
    _listTsCam.push_back(pCam);

    std::thread objectThread_1(thread_do_detect_1);
    std::thread objectThread_2(thread_do_detect_2);
    std::thread objectThread_3(thread_do_detect_3);
    std::thread objectThread_4(thread_do_detect_4);
    std::thread objectThread_5(thread_do_detect_5);
    std::thread objectThread_6(thread_do_detect_6);

    while ( true )
    {
        sleep ( 3 );
    }

    TsMulitGstCamPlayer::GstEnvDeinit();

}

void snpe_entities_init()
{  
    labels =  new std::string[1000];
    std::ifstream in("../models/labels.txt");
    std::string line;
    int count=0;
    while (getline(in,line)){
        labels[count] = line;
        count++;
    }

    DSP_Detector_1 = new YoloClassification();
    DSP_Detector_2 = new YoloClassification();
    DSP_Detector_3 = new YoloClassification();
    DSP_Detector_4 = new YoloClassification();
    DSP_Detector_5 = new YoloClassification();
    DSP_Detector_6 = new YoloClassification();

    DSP_Detector_1->init(DSP);
    DSP_Detector_1->setConfidence(0.7f);
    DSP_Detector_2->init(DSP);
    DSP_Detector_2->setConfidence(0.7f);
    DSP_Detector_3->init(DSP);
    DSP_Detector_3->setConfidence(0.7f);
    DSP_Detector_4->init(DSP);
    DSP_Detector_4->setConfidence(0.7f);
    DSP_Detector_5->init(DSP);
    DSP_Detector_5->setConfidence(0.7f);
    DSP_Detector_6->init(DSP);
}

int main ( int argc, char* argv[] )
{
    printf("\n[cmd:]  app \n ");

    snpe_entities_init();
    start_task();

    return 0;
}