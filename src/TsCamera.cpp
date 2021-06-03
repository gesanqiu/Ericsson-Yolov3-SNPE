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
#include <opencv2/imgcodecs/imgcodecs_c.h>


#define OBJECT_DETECT_MAX   2

#define AI_RES_WIDTH 1920
#define AI_RES_HEIGHT 1080

#define GST_RTSPSRC_PATH   "GST_RTSPSRC_PATH"

std::string s_str_object_set[OBJECT_DETECT_MAX] = {"object1",
    "object2"};

static float same_threshold = 0.6;


GstElement* TsMulitGstCamPlayer::g_pPipeline = NULL;
GstElement* TsMulitGstCamPlayer::g_pOutPipeline = NULL;

TsProAndCon<cv::Mat> rgb_frame_cache;
std::vector<TsCamera*> _listTsCam;

TsCamera* TsMulitGstCamPlayer::buildCamById ( int nId, std::string strName, int nWidth,
                                              int nHeight, std::string strDecodeType, std::string strUri, int nFramerate )
{
    /*TsCamera* pCam = new TsCamera();
    pCam->SetName ( strName );
    pCam->SetWidth ( nWidth );
    pCam->SetHeight ( nHeight );
    pCam->SetDecodeType ( strDecodeType );
    pCam->SetCameraID ( nId );
    pCam->SetFramerate ( nFramerate );
    pCam->SetUri ( strUri );
    pCam->SetAiWay("facedect");
    pCam->BuildPipeLine ( true, true );
    pCam->SetEnable ( true );
    pCam->Init (  );
    return pCam;*/
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
    show_frame_cache.debug_info = "show_frame_cache";
    frame_cache.debug_info = "frame_cache";

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

    /* create a Display pipeline */
    m_pOutPipeline = gst_parse_launch ( m_stroutline.c_str(), &error );

    if ( error != NULL )
    {
        printf ( "[Camera %s]could not construct pipeline: %s\n", GetName().c_str(), error->message);
        g_clear_error ( &error );
        goto exit;
    }

    printf ( "[Camera------------------1------------------------]\n");
    /* get sink */
    m_pAppSrcSink = gst_bin_get_by_name ( GST_BIN ( m_pOutPipeline ), GetAppSrcName().c_str() );

    /* Putting a Message handler */
    m_pOutBus = gst_pipeline_get_bus ( GST_PIPELINE ( m_pOutPipeline ) );
    gst_bus_add_watch ( m_pOutBus, MY_BUS_CALLBACK, reinterpret_cast<void*> ( this ) );
    gst_object_unref ( m_pOutBus );

    /* appsrc sink */
    g_signal_connect (m_pAppSrcSink, "need-data", G_CALLBACK (cbNeedData), reinterpret_cast<void*> ( this ));

    printf ( "[Camera------------------22------------------------]\n");
    /* Run the pipeline */
    printf ( "[Camera %s]Display: %s\n", GetName().c_str(), GetUri().c_str() );
    printf ( "[Camera------------------23------------------------]\n");
    gst_element_set_state ( m_pOutPipeline, GST_STATE_PLAYING );

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
    if ( dec == NULL || appsink == NULL )
    {
        printf ( "[Camera %s]decode or appsink is null\n", dec->GetName().c_str() );
        return GST_FLOW_OK;
    }

    //sample = gst_app_sink_pull_sample(appsink);
    sample = gst_base_sink_get_last_sample ( GST_BASE_SINK ( appsink ) );
    //printf ("[Camera %s]pull sample clock time is : %ld\n", dec->GetName().c_str(), gst_base_sink_get_latency(GST_BASE_SINK(appsink)));
    if ( sample == NULL )
    {
        printf ( "[Camera %s]pull sample is null\n", dec->GetName().c_str() );
    }
    else
    {
        dec->frame_cache.product(std::shared_ptr<GstSample> ( sample, deleterGstSample ));
    }

    return GST_FLOW_OK;
}

void TsCamera::BuildPipeLine ( bool rtsp, bool isHwDec )
{
    std::ostringstream cameraPath;
    std::ostringstream cameraOutPath;
    int width = AI_RES_WIDTH;
    int height = AI_RES_HEIGHT;
    cameraPath << "rtspsrc location=" << GetUri() << " latency=0 tcp-timeout=500 drop-on-latency=true ntp-sync=true" << " ! ";
    cameraPath << "queue ! rtp" << GetDecodeType() << "depay ! "<< GetDecodeType() << "parse ! queue ! qtivdec " << " ! ";
    cameraPath << "queue ! qtivtransform ! video/x-raw,format=BGR,width=" << width << ",height=" << height;
    cameraPath << " ! appsink name=" << GetName() << " sync=false ";
    cameraPath << "caps=video/x-raw,format=BGR,width=" << width << ",height=" << height << " ";

    cameraOutPath << "appsrc name=" << GetAppSrcName() << " stream-type=0 format=3 is-live=true ";
    cameraOutPath << " caps=video/x-raw,format=BGR,width=" << width << ",height=" << height << " ! ";
    cameraOutPath << "videoconvert ! waylandsink name=" << GetWaylandName() << " sync=false x=" << w_show_x << " y=" << w_show_y << " width=" << w_show_width << " height=" << w_show_height;

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

    frame_cache.consumption(dst);

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

void TsCamera::cbNeedData (GstElement *appsrc,
                  guint       unused_size,
                  gpointer    user_data)
{
    TsCamera* dec = reinterpret_cast<TsCamera*> ( user_data );
    static GstClockTime timestamp = 0;
    GstBuffer *buffer;
    GstFlowReturn ret;

    std::shared_ptr<cv::Mat> imgframe;
    dec->show_frame_cache.consumption(imgframe);
    if ( imgframe == NULL || imgframe.get() == NULL )
    {
        imgframe = std::make_shared<cv::Mat>(AI_RES_HEIGHT, AI_RES_WIDTH, CV_8UC3, cv::Scalar(0, 0, 0));
    }

    int len = imgframe->total() * imgframe->elemSize();
    buffer = gst_buffer_new_allocate (NULL, len, NULL);

    {
        int left = 0;
        int top = 0;

        std::deque<S_OBJECT_DATA> objectlist =  dec->getObjectInfoList();
        int size = objectlist.size();

        for (unsigned int i = 0; i < size; i++ )
        {
            S_OBJECT_DATA data = objectlist[i];

            if (data.score >= same_threshold)
            {
                cv::Scalar color(150, 255, 40);
                std::string words= " " + s_str_object_set[data.label];
                //std::cout << "["<<  __func__ << __LINE__ << "]"
                //  << " Check Out Object Name:" << words << std::endl;
                cv::Point frontpos= cv::Point ( ( int ) ( data.x+left ), ( int ) ( data.y+top - 10 ) );
                cv::putText(*imgframe, words, frontpos, cv::FONT_HERSHEY_COMPLEX, 0.8, color,2,0.3);
            }

            // merge face box;
            //LOGD("fd(%d) x=%d, y=%d , width=%d, height=%d", k, (int)(data.x), (int)(data.y), (int)(data.width), (int)(data.height));
            //cv::rectangle(*imgframe, cv::Rect((int)(data.x+left), (int)(data.y+top), (int)data.width, (int)data.height), cv::Scalar(0, 200, 0), 3);
            int grapsize = 15;
            int thickness=2;
            cv::Scalar scalar ( 0, 200, 0 );
            cv::line ( *imgframe, cv::Point ( ( int ) ( data.x+left ), ( int ) ( data.y+top ) ), cv::Point ( ( int ) ( data.x+grapsize+left ), ( int ) ( data.y+top ) ), scalar, thickness );
            cv::line ( *imgframe, cv::Point ( ( int ) ( data.x+left ), ( int ) ( data.y+top ) ), cv::Point ( ( int ) ( data.x+left ), ( int ) ( data.y+grapsize+top ) ), scalar, thickness );
            cv::line ( *imgframe, cv::Point ( ( int ) ( data.x+left + data.width - grapsize ), ( int ) ( data.y+top ) ), cv::Point ( ( int ) ( data.x+left + data.width ), ( int ) ( data.y+top ) ), scalar, thickness );
            cv::line ( *imgframe, cv::Point ( ( int ) ( data.x+left + data.width ), ( int ) ( data.y+top ) ), cv::Point ( ( int ) ( data.x+left + data.width ), ( int ) ( data.y+grapsize+top ) ), scalar, thickness );

            cv::line ( *imgframe, cv::Point ( ( int ) ( data.x+left ), ( int ) ( data.y+data.height-grapsize+top ) ), cv::Point ( ( int ) ( data.x+left ), ( int ) ( data.y+data.height+top ) ), scalar, thickness );
            cv::line ( *imgframe, cv::Point ( ( int ) ( data.x+left ), ( int ) ( data.y+data.height+top ) ), cv::Point ( ( int ) ( data.x+grapsize+left ), ( int ) ( data.y+data.height+top ) ), scalar, thickness );
            cv::line ( *imgframe, cv::Point ( ( int ) ( data.x+left-grapsize+ data.width ), ( int ) ( data.y+data.height+top ) ), cv::Point ( ( int ) ( data.x+left+ data.width ), ( int ) ( data.y+data.height+top ) ), scalar, thickness );
            cv::line ( *imgframe, cv::Point ( ( int ) ( data.x+left+ data.width ), ( int ) ( data.y+data.height+top ) ), cv::Point ( ( int ) ( data.x+left+ data.width ), ( int ) ( data.y-grapsize+data.height+top ) ), scalar, thickness );
        }
    }

    /* this makes the image */
    GstMapInfo map;
    gst_buffer_map(buffer,&map,GST_MAP_READ);
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
    //usleep ( 50*1000 );
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

//gst-launch-1.0 rtspsrc location="rtsp://10.0.36.254:554/user=admin&password=&channel=1&stream=0.sdp?" latency=0 ! queue ! rtph264depay ! h264parse ! queue ! vaapih264dec low-latency=true ! vaapipostproc width=1920 height=1080 format=16 ! appsink sync=false

/**
 * [thread_cam_convert convert task NV12/I420 to RGB]
 * @Author   zhaoyl
 * @param    void
 */
void thread_cam_convert ( void )
{
    while ( true )
    {
        for (unsigned int k  = 0; k < _listTsCam.size(); k++ )
        {
            std::shared_ptr<GstSample> sample;
            std::shared_ptr<cv::Mat> imgframe;
            GstBuffer* buffer = NULL;
            GstCaps* caps = NULL;
            const GstStructure* info = NULL;
            GstMapInfo map;
            int sample_width = 0;
            int sample_height = 0;
            TsCamera* pCam = _listTsCam.at ( k );

            if ( pCam->GetName().compare ( "unkown" ) != 0 )
            {
                pCam->CatureConWait ( sample );
                if ( sample == NULL|| sample.get() == NULL )
                {
                    continue;
                }
                buffer = gst_sample_get_buffer ( sample.get() );
                if ( buffer == NULL )
                {
                    printf ( "[Camera %s]get buffer is null\n", pCam->GetName().c_str() );
                    continue;
                }

                gst_buffer_map ( buffer, &map, GST_MAP_READ );

                caps = gst_sample_get_caps ( sample.get() );
                if ( caps == NULL )
                {
                    printf ( "[Camera %s]get caps is null\n", pCam->GetName().c_str() );
                    continue;
                }

                info = gst_caps_get_structure ( caps, 0 );
                if ( info == NULL )
                {
                    printf ( "[Camera %s]get info is null\n", pCam->GetName().c_str() );
                    continue;
                }

                // ---- Read frame and convert to opencv format ---------------

                // convert gstreamer data to OpenCV Mat, you could actually
                // resolve height / width from caps...
                gst_structure_get_int ( info, "width", &sample_width );
                gst_structure_get_int ( info, "height", &sample_height );
                cv::Mat tmpmat ( sample_height, sample_width, CV_8UC3, ( unsigned char* ) map.data, cv::Mat::AUTO_STEP );
                tmpmat = tmpmat.clone();
                pCam->show_frame_cache.product(std::make_shared<cv::Mat>(tmpmat));
                rgb_frame_cache.product(std::make_shared<cv::Mat>(tmpmat));
                gst_buffer_unmap ( buffer, &map );
                // show caps on first frame
                if ( !pCam->m_bPrintStreamInfo )
                {
                    printf ( "[Camera %s]%s\n", pCam->GetName().c_str(), gst_caps_to_string ( caps ) );
                    pCam->m_bPrintStreamInfo = true;
                }
            }
        }

        //usleep ( 40*1000 );

        // detect ...
    }
}
/**
 * [task_cpu_convert set thread cpu 0]
 * @Author   zhaoyl
 * @param    void
 */
void* task_cpu_convert ( void* argc )
{
    int i, cpus = 0;
    cpu_set_t mask;
    cpu_set_t get;

    cpus = sysconf ( _SC_NPROCESSORS_CONF );
    printf ( "this system has %d processor(s)\n", cpus );

    CPU_ZERO ( &mask );
    CPU_SET ( 0, &mask );

    if ( pthread_setaffinity_np ( pthread_self(), sizeof ( mask ), &mask ) < 0 )
    {
        fprintf ( stderr, "set thread affinity failed\n" );
    }

    CPU_ZERO ( &get );
    if ( pthread_getaffinity_np ( pthread_self(), sizeof ( get ), &get ) < 0 )
    {
        fprintf ( stderr, "get thread affinity failed\n" );
    }

    for ( i = 0; i < cpus; i++ )
    {
        if ( CPU_ISSET ( i, &get ) )
        {
            printf ( "this thread %d is running in processor %d\n", ( int ) pthread_self(), i );
        }
    }

    thread_cam_convert();
    pthread_exit ( NULL );
}

void thread_cam_object(void)
{
    while(true)
    {
        std::shared_ptr<cv::Mat> imgframe;
        rgb_frame_cache.consumption(imgframe);
        std::vector<S_OBJECT_DATA> vec_object_rect;
        //TODO AI SDK detect

        //std::cout << "["<< __FILE__ << __func__ << __LINE__ << "]" << vec_object_rect.size() << std::endl;

        for (unsigned int k  = 0; k < _listTsCam.size(); k++ )
        {
            TsCamera* pCam = _listTsCam.at ( k );
            if ( pCam->GetName().compare ( "unkown" ) == 0 )
            {
                continue;
            }
            pCam->clearObjectList();
            for (unsigned int i = 0; i < vec_object_rect.size(); i++ )
            {
                pCam->addObjectInfo ( vec_object_rect[i] );
            }
        }
        //usleep(50*1000);
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


    rgb_frame_cache.debug_info = "rgb_frame_cache";

    char* sourcepath = getenv(GST_RTSPSRC_PATH);
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

    std::thread yuvconvertThread(thread_cam_convert);
    //pthread_t tid_convert;
    //pthread_create ( &tid_convert, NULL, task_cpu_convert, NULL );

    std::thread objectThread(thread_cam_object);

    while ( true )
    {
        sleep ( 3 );
    }

    TsMulitGstCamPlayer::GstEnvDeinit();

}


int main ( int argc, char* argv[] )
{
    printf("\n[cmd:]  app \n ");
    start_task();

    return 0;
}



