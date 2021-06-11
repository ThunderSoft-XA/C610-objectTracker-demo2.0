#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include "gstcamera.h"
// #include "libyuv/convert.h"
// #include "libyuv/planar_functions.h"
// #include "libyuv/scale_argb.h"

using namespace std;

namespace gstcamera{

GstElement *MulitGstCamera::g_pPipeline = NULL;

GstCamera::GstCamera()
{

}

void GstCamera::Init(GstElement *_pipeline)
{
    this->set_index(0);
    this->pipeline = _pipeline;
    this->pipeline = gst_pipeline_new (this->get_pipe_name().c_str());

    gst_time_start.tv_sec = 0;
    gst_time_start.tv_usec = 0;
    this->sample_ready = false;
    this->convert_finished = true;
    this->track_finished = true;
    this->show_finished = true;

    this->stream_end = false;

    frame_cache = std::make_shared<BufManager<GstSample> > ();
    error = NULL;
    if(this->pipe_name.empty()) {
        set_pipe_name("qmmf");
    }
    if(this->sink_name.empty()) {
        set_sink_name("app_sink");
    }
    DEBUG_FUNC();
    if(GstType::LOCALFILE == this->gst_type) {
        BuildLocalPipeLine(this->hw_dec);
    } else if (GstType::CAMERA == this->gst_type) {
        BUildCameraPipeLine(this->hw_dec);
    } else if (GstType::RTSP == this->gst_type) {
        BuildRtspPipeLine(this->hw_dec);
    }

    this->HandleAppsink();

       /* Run the pipeline for Start playing */
    DEBUG_FUNC();
    if (GST_STATE_CHANGE_FAILURE == gst_element_set_state (this->pipeline, GST_STATE_PLAYING)) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
    }

    /* Putting a Message handler */
    DEBUG_FUNC();
    this->gstbus = gst_pipeline_get_bus (GST_PIPELINE (this->pipeline));
    gst_bus_add_watch (this->gstbus, MY_BUS_CALLBACK, reinterpret_cast<void *>(this));
    gst_object_unref (this->gstbus);
}

void GstCamera::HandleAppsink()
{
    /* get sink */
    this->appsink = gst_bin_get_by_name (GST_BIN (this->pipeline), this->get_sink_name().c_str());
    std::cout << "appsink name = " << gst_object_get_name(GST_OBJECT(this->appsink)) << std::endl;

    /*set sink prop*/
    gst_app_sink_set_emit_signals((GstAppSink*)this->appsink, true);
    gst_app_sink_set_drop((GstAppSink*)this->appsink, true);
    gst_app_sink_set_max_buffers((GstAppSink*)this->appsink, 1);
    gst_base_sink_set_sync(GST_BASE_SINK(this->appsink),false);
    gst_base_sink_set_last_sample_enabled(GST_BASE_SINK(this->appsink), true);
    //gst_base_sink_set_drop_out_of_segment(GST_BASE_SINK(this->appsink), true);
    gst_base_sink_set_max_lateness(GST_BASE_SINK(this->appsink), 0);

    {//avoid goto check
        GstAppSinkCallbacks callbacks = { onEOS, onPreroll, onBuffer };
        gst_app_sink_set_callbacks (GST_APP_SINK(this->appsink), &callbacks, reinterpret_cast<void *>(this), NULL);
    }
}

bool GstCamera::ForPipeLine(string _pipeline_str)
{
    this->pipeline = gst_parse_launch(_pipeline_str.c_str(),&this->error);
    if (this->error != NULL) {
        printf ("could not construct pipeline: %s\n", error->message);
        g_clear_error (&error);
        goto exit;
    }
    return true;
exit:
    if(this->pipeline != NULL) {
        gst_element_set_state (this->pipeline, GST_STATE_NULL);
        gst_object_unref (this->pipeline);
        this->pipeline = NULL;
    }
    return false;
}

void GstCamera::BuildLocalPipeLine(bool isHwDec)
{
    std::ostringstream cameraPath;
    cameraPath << "filesrc location=" << get_path() << " ! " << "qtdemux name=demux demux. ! queue ! h264parse ! ";
    if(isHwDec) {
        cameraPath << "omx" << get_decode_type() << "dec " << " ! ";
    } else {
        cameraPath << "avdec_" << get_decode_type() << " ! ";
    }
    cameraPath << "videoscale ! video/x-raw,width=" << get_width() << ",height=" << get_height() << " ! appsink name=" << get_sink_name() << " sync=false  max-lateness=0 max-buffers=1 drop=true";
    this->pipeline_str = cameraPath.str();
    this->ForPipeLine(this->pipeline_str);
}

void GstCamera::BuildRtspPipeLine(bool isHwDec)
{
    std::ostringstream cameraPath;
    cameraPath << "rtspsrc location=" << get_path() << " latency=0 tcp-timeout=500 drop-on-latency=true ntp-sync=true" << " ! ";
    cameraPath << "queue ! rtp" << get_decode_type() << "depay ! "<< get_decode_type() << "parse ! queue ! ";
    if(isHwDec) {
        cameraPath << "omx" << get_decode_type() << "dec " << " ! ";
    } else {
        cameraPath << "avdec_" << get_decode_type() << " ! ";
    }
    cameraPath << "videoscale ! video/x-raw,width=" << get_width() << ",height=" << get_height() <<  " ! appsink name=" << get_sink_name() << " sync=false  max-lateness=0 max-buffers=1 drop=true";
    this->ForPipeLine(cameraPath.str());
}

void GstCamera::BUildCameraPipeLine(bool isHwDec)
{
    std::ostringstream cameraPath;
    cameraPath << "qtiqmmfsrc af-mode=auto name="<< get_pipe_name() << " !";
    cameraPath << "video/x-"<< get_decode_type() << ",format=" << get_format() << ",width="<< get_width() << ",height="<< get_height() <<",framerate="<< get_framerate() <<"/1" << " ! ";
    if(isHwDec) {
        cameraPath << get_decode_type() << "parse ! queue ! qtivdec ! qtivtransform rotate=90CW ! video/x-raw,format=BGRA ! ";
    } else {
        cameraPath << "queue ! avdec_" << get_decode_type() <<" ! videoscale";
    }
    cameraPath << "appsink name=" << get_sink_name() << " sync=false  max-lateness=0 max-buffers=1 drop=true";
    this->ForPipeLine(cameraPath.str());
}

// onEOS
void GstCamera::onEOS(GstAppSink *appsink, void *user_data)
{
    GstCamera *dec = reinterpret_cast<GstCamera *>(user_data);
    dec->stream_end = true;
    printf("gstreamer decoder onEOS\n");
}

// onPreroll
GstFlowReturn GstCamera::onPreroll(GstAppSink *appsink, void *user_data)
{
    GstCamera *dec = reinterpret_cast<GstCamera *>(user_data);
    printf("gstreamer decoder onPreroll\n");
    return GST_FLOW_OK;
}

void GstCamera::gst_pull_block()
{
	std::unique_lock<std::mutex> lk(_gstTimerMut);
	_gstTimerCond.wait(lk);
}

void GstCamera::gst_pull_continue()
{
    std::unique_lock<std::mutex> lk(_gstTimerMut);
	_gstTimerCond.notify_all();
}

static void deleterGstSample(GstSample* x) {
    //std::cout << "DELETER FUNCTION CALLED\n";
    if(x != NULL) {
        gst_sample_unref (x);
    }
}

// onBuffer
GstFlowReturn GstCamera::onBuffer(GstAppSink *appsink, void *user_data)
{
    DEBUG_FUNC();
    GstCamera *dec = NULL;
    GstSample *sample = NULL;
    double elapsed_time = 0.0;
    struct timeval one_buffer_time_end;
    long on_buffer_pre_time = 0;
    dec = reinterpret_cast<GstCamera *>(user_data);
    if(dec == NULL || appsink == NULL) {
        printf ("decode or appsink is null\n");
        return GST_FLOW_ERROR;
    }
    while(!dec->show_finished) {
        dec->gst_pull_block();
    }
    if(!dec->get_index()) gettimeofday(&dec->gst_time_start,nullptr);

    // sample = gst_app_sink_pull_sample(appsink);
	sample = gst_base_sink_get_last_sample(GST_BASE_SINK(appsink));
    if(sample == NULL) {
        printf ("pull sample is null\n");
    } else {
        dec->frame_cache->feed(std::shared_ptr<GstSample>(sample, deleterGstSample));
        dec->set_index(dec->get_index()+1);
    }
    dec->sample_ready = true;
    dec->convert_finished = false;
    dec->track_finished = false;
    dec->show_finished = false;
    dec->gst_pull_continue();

    DEBUG_FUNC();
    on_buffer_pre_time = getCurrentTime();
    gettimeofday(&one_buffer_time_end,nullptr);
    elapsed_time = (one_buffer_time_end.tv_sec - dec->gst_time_start.tv_sec) * 1000 +
                                (one_buffer_time_end.tv_usec - dec->gst_time_start.tv_usec) / 1000;
    if(elapsed_time > 1000) {
		if(dec->get_index()*1000/elapsed_time < dec->get_framerate())
            printf("%s : framerate=%f\n", GetLocalTimeWithMs().c_str(),dec->get_index()*1000/elapsed_time);
        dec->set_index(0);
    }
    DEBUG_FUNC();
    return GST_FLOW_OK;
}

gboolean GstCamera::MY_BUS_CALLBACK (GstBus *bus, GstMessage *message, gpointer data) 
{
    GstCamera *_gst_camera = reinterpret_cast<GstCamera *>(data);
    switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR: {
        GError *err;
        gchar *debug;

        gst_message_parse_error (message, &err, &debug);
        g_print ("Error: %s\n", err->message);
        g_error_free (err);
        g_free (debug);

        gst_element_set_state(_gst_camera->pipeline,GST_STATE_READY);
        break;
    }
    case GST_MESSAGE_EOS:
        /* end-of-stream */
        gst_element_set_state(_gst_camera->pipeline,GST_STATE_READY);
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

void GstCamera::CaptureNoWait(std::shared_ptr<GstSample>& dst)
{
    DEBUG_FUNC();
    dst = frame_cache->fetch();
}

void GstCamera::addTrackInfo(TrackInfo _track_Info)
{
    std::unique_lock<std::mutex> lk(track_queue_mut);
    track_info_queue.push_back(_track_Info);
}

void GstCamera::clearTrackQueue()
{
    std::unique_lock<std::mutex> lk(track_queue_mut);
    track_info_queue.clear();
}

std::deque<TrackInfo> GstCamera::getTrackInfo()
{
    std::unique_lock<std::mutex> lk(track_queue_mut);
    std::deque<TrackInfo> list = track_info_queue;
    return list;
}

void GstCamera::Deinit()
{
    if(appsink!=NULL)
    {
    }
    if(pipeline!=NULL) {
        gst_element_set_state (pipeline, GST_STATE_NULL);
        gst_object_unref (pipeline);
        pipeline = NULL;
    }
}

GstCamera::~GstCamera()
{
}

}