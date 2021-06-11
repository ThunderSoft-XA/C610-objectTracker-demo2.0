#ifndef __GST_CAMERA_H__
#define __GST_CAMERA_H__

#include <iostream>
#include <sstream>
#include <condition_variable>
#include <opencv2/opencv.hpp>
#include <gst/app/app.h>
#include <cairo/cairo.h>
#include "utils/timeutil.h"
#include "publicattr.h"
#include "bufmanager.hpp"

namespace gstcamera{

class MulitGstCamera
{
private:
    static GstElement *g_pPipeline;

public:
    static void GstEnvInit() {
        gst_init (NULL, NULL);
    }

    static GstElement *GetPipeline() {
        return g_pPipeline;
    }
    static void GstEnvDeinit() {
        if(g_pPipeline != NULL)
        {
            gst_element_set_state (g_pPipeline, GST_STATE_NULL);
            gst_object_unref (g_pPipeline);
            g_pPipeline = NULL;
        }
    }
};

class GstCamera : public VideoAttr{
private:
    string pipeline_str;
    string pipe_name;
    string sink_name;
    GstType gst_type;
    AIType ai_type;
    bool hw_dec;

    GstElement *pipeline,*appsink;
    GstBus *gstbus;
    GError *error;

    struct timeval gst_time_start;

    std::shared_ptr<BufManager<GstSample> > frame_cache;

    std::mutex track_queue_mut;
    std::deque<TrackInfo> track_info_queue;

    void BuildLocalPipeLine(bool isHwDec);
    void BuildRtspPipeLine(bool isHwDec);
    void BUildCameraPipeLine(bool isHwDec);
    bool ForPipeLine(string _pipeline_str);
    void HandleAppsink();
    
    static void onEOS(GstAppSink *appsink, void *user_data);
    static GstFlowReturn onPreroll(GstAppSink *appsink, void *user_data);
    static GstFlowReturn onBuffer(GstAppSink *appsink, void *user_data);

    static gboolean MY_BUS_CALLBACK (GstBus *bus, GstMessage *message, gpointer data);

public:
    GstCamera();
    GstCamera(int argc, char **argv);
    ~GstCamera();

    bool sample_ready;
    bool convert_finished;
    bool track_finished;
    bool show_finished;
    bool stream_end;
    float trackPS;

    TrackInfo track_info;
    vector<cv::Point2f> center_point;
    std::shared_ptr<BufManager<cv::Mat>> rgb_object_frame;

    std::mutex _gstTimerMut;
    std::condition_variable _gstTimerCond;

    void Init(GstElement *_pipeline);
    void Deinit();
    void gst_pull_block();
    void gst_pull_continue();
    void CaptureNoWait(std::shared_ptr<GstSample>& dst);
    void addTrackInfo(TrackInfo _track_Info);
    void clearTrackQueue();

    std::deque<TrackInfo> getTrackInfo();

    string get_pipeline_str() {
        return this->pipeline_str;
    }

    void set_pipe_name(string _name) {
        this->pipe_name = _name;
    }
    string get_pipe_name(){
        return this->pipe_name;
    }

    void set_sink_name(string _name) {
        this->sink_name = _name;
    }
    string get_sink_name(){
        return this->sink_name;
    }

    void set_gst_type(GstType _type) {
        this->gst_type = _type;
    }
    GstType get_gst_type() {
        return this->gst_type;
    }

    void set_ai_type(AIType _type) {
        this->ai_type = _type;
    }
    AIType get_ai_type() {
        return this->ai_type;
    }

    void set_hw_dec(bool _hw) {
        this->hw_dec = _hw;
    }
    bool is_hwdec() {
        return this->hw_dec;
    }

};

}

#endif /*__GST_CAMERA_H__*/