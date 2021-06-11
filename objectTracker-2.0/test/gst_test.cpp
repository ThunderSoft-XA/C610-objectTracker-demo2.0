#include <iostream>
#include <mcheck.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <thread>

#include "gstcamera.h"
#include "libyuv.h"
#include "objecttracker.h"
#include "utils/configenv.hpp"
#include "config/include/param_ops.h"

#include <gst/gst.h>
#include <gst/app/app.h>

using namespace objecttracker;
using namespace gstcamera;
using namespace libyuv;
using namespace std;

#define PICTURE_WIDTH 640
#define PICTURE_HEIGHT 360
#define FRAME_PER_SECOND 30
#define FORMAT_TYPE "NV12"

#define SHOW_SLEEP_TIME 1

#define TARGET_PATH "/home/user/Desktop/test/objectTracker/object.png"

#define SUPPORT_AI 1
#define SHOW_VIDEO 1
#define DEBUG_INFO 1


std::vector<GstCamera *> _listGstCam;

std::vector<string> CLASSES = {"background", "aeroplane", "bicycle", "bird", "boat",
	"bottle", "bus", "car", "cat", "chair", "cow", "diningtable",
	"dog", "horse", "motorbike", "person", "pottedplant", "sheep",
	"sofa", "train", "tvmonitor"};

static const struct option long_option[]={
   {"object",optional_argument,NULL,'i'},
   {"video",optional_argument,NULL,'v'},
   {"model",optional_argument,NULL,'m'},
   {NULL,0,NULL,0}
};

int parm_parse(int argc,char *argv[])
{
    int opt=0;
    while((opt = getopt_long(argc,argv,"i:l",long_option,NULL)) != -1) {
        switch(opt) {
            case 'i':break;
            case 'v':
                printf("name:%s ", optarg);
                break;
            case 'm': 
                break;
        }
    }
}

void preview(std::shared_ptr<cv::Mat> imgframe)
{
  cv::Mat showframe;
#if 1 
        cv::resize(*imgframe, showframe, cv::Size(1920,1080), 0, 0, cv::INTER_LINEAR);
        cv::imshow("sink", showframe);
        cv::waitKey(1);
#endif
		return ;
}

#define SSTR( x ) static_cast< std::ostringstream & >( \
( std::ostringstream() << std::dec << x ) ).str()

static void thread_show(void)
{
    // Multichannel display block diagram segmentation calculation
    int num_w = sqrt(_listGstCam.size());
    num_w = num_w + (pow(num_w, 2) < _listGstCam.size() ? 1 : 0);
    int num_h = _listGstCam.size()/num_w + (_listGstCam.size()%num_w > 0 ? 1 :0);
    cout << "nuw_w = " << num_w << "num_h =" << num_h << endl;

    int width = 640;
    int height = 360;
    int show_left,show_top;
    DEBUG_FUNC();
    while(true) {
        std::shared_ptr<cv::Mat> img_show;
        std::shared_ptr<cv::Mat> sub_imgframe;
        cv::Mat imageShow(cv::Size(width*num_w, height*num_h), CV_8UC3);
        img_show = std::make_shared<cv::Mat>(imageShow);

        DEBUG_FUNC();
        for(int i = 0; i < _listGstCam.size(); i++) {
            GstCamera* pCam = _listGstCam.at(i);
            show_left = i % num_w * (width);
            show_top = i / num_w * (height); 
            if(true == pCam->stream_end) {
                continue;
            }
            if(pCam->track_finished != true) {
                pCam->gst_pull_block();
            }

            DEBUG_FUNC();
            sub_imgframe = pCam->rgb_object_frame->fetch();
            if(sub_imgframe == NULL || sub_imgframe.get() == NULL) {
                continue;
            }
            if( AIType::TARGETTRACK == pCam->get_ai_type()) {
                // std::deque<TrackInfo> track_result_que = pCam->getTrackInfo()
                // if(track_result_que.empty()) {
                //     pCam->gst_pull_block();
                // }

                TrackInfo track_info = pCam->track_info;
                // int size = track_result_que.size();
                // cout << "track result deque size =" << size << endl;
                if( !track_info.track_data.empty()) {
                    DEBUG_FUNC();
                    std::vector<cv::Rect2f> result_vec = track_info.track_data;
                    if( result_vec.size() > 0) {
                        for(int i = 0; i < result_vec.size(); i++) {
                            cv::Rect2f result_rect = result_vec[i];
                            cv::rectangle(*sub_imgframe, (cv::Rect)result_rect, cv::Scalar(0, 200, 0));
                            cv::putText(*sub_imgframe,track_info.target_name, cv::Point(result_rect.x, result_rect.y-15), cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(50, 170, 50), 1);
                            cv::putText(*sub_imgframe,to_string(track_info.confidence), cv::Point(result_rect.x + 50, result_rect.y-15), cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(50, 170, 50), 1);
                        }
                    }
                    if(pCam->center_point.size() > 1) {
                        for(int i =0; i < pCam->center_point.size() -1; i++) {
                            cv::line(*sub_imgframe,pCam->center_point[i],pCam->center_point[i+1],cv::Scalar(89, 90, 90),3);
                        }

                    }
                }
                cv::putText(*sub_imgframe, "Tracker", cv::Point(50, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(50, 170, 50), 1);
                cv::putText(*sub_imgframe, "FPS : " + SSTR(int(pCam->get_framerate())), cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(50, 170, 50), 1);
                cv::putText(*sub_imgframe, "OTPS : " + SSTR(int(pCam->trackPS)), cv::Point(50,80), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(50, 170, 50), 1);
                sub_imgframe->copyTo(imageShow(cv::Rect(show_left,show_top,width,height)));
            } else if(AIType::OBJECTDETECT == pCam->get_ai_type()) {
                // do nothing 
            } else if(AIType::POSE == pCam->get_ai_type()) {
                // tempoary do nothing
            } else {
                cv::putText(*sub_imgframe, "FPS : " + SSTR(int(pCam->get_framerate())), cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, 0.75, cv::Scalar(50, 170, 50), 1);
                sub_imgframe->copyTo(imageShow(cv::Rect(show_left,show_top,width,height)));    
            }
            pCam->sample_ready = false;
            pCam->convert_finished = false;
            pCam->track_finished = false;
            pCam->show_finished = true;
            pCam->gst_pull_continue();
        }
        DEBUG_FUNC();
        if(img_show->empty() != true && img_show.get() != NULL)
                preview(img_show);
    }
}

//This thread is only for target tracking
static void thread_object_track(void)
{
    ObjectTracker *my_tracker;
    my_tracker = new ObjectTracker(runtime_t::CPU);
    my_tracker->setSiam(false);
    my_tracker->setConfidence(0.9);

    struct timeval time_start,time_end;
	int track_frame_count=0;
    gettimeofday(&time_start, nullptr);

    DEBUG_FUNC();
    while(true) {
        std::vector<float> result_vec;
        cv::Rect2f result_rect;

        for(int i = 0; i < _listGstCam.size(); i++) {
            GstCamera* _gst_camera = _listGstCam.at(i);

            if(true == _gst_camera->stream_end) {
                continue;
            }
            if(_gst_camera->convert_finished != true) {
                _gst_camera->gst_pull_block();
            }

            DEBUG_FUNC();
            std::vector<float> result;
            TrackInfo track_info;
            assert(_gst_camera->rgb_object_frame != NULL);
            DEBUG_FUNC();
            if(_gst_camera->rgb_object_frame->fetch()->empty() == true) {
                continue;
            }
            DEBUG_FUNC();
            // get a vetor of mat from object `s    mat and video frame
            // get a rect point result by exexcutesnpe network
            if(AIType::TARGETTRACK == _gst_camera->get_ai_type()) {
                std::cout << __FILE__ << "==entering doTracking==" << __LINE__ << std::endl;
                if(false == my_tracker->getSiam()) {
                    result = my_tracker->doTracking(*_gst_camera->rgb_object_frame->fetch());
                } else {
                    // do nothing
                }
                track_info.target_name = CLASSES[result[1]];
                track_info.confidence = result[2];
                result_rect = cv::Rect2f(cv::Point2f(result[3]*640,result[4]*360),cv::Point2f(result[5]*640,result[6]*360));
                track_info.track_data.push_back(result_rect);
                _gst_camera->center_point.push_back(cv::Point2f(result[3]*640 + (result[5]*640 - result[3]*640)/2,result[4]*360 + (result[6]*360 - result[4]*360)/2));
                // DEBUG_FUNC();
                // _gst_camera->addTrackInfo(track_info);
                _gst_camera->track_info = track_info;
        }

#if DEBUG_INFO
            track_frame_count++;
            gettimeofday(&time_end, nullptr);
            elapsed_time = (time_end.tv_sec - time_start.tv_sec) * 1000 +
                (time_end.tv_usec - time_start.tv_usec) / 1000;
            if(elapsed_time > 1000*10) {
                printf("[show] showframerate=%f\n", track_frame_count*1000/elapsed_time);
                // for record track rate
                _gst_camera->trackPS =  track_frame_count*1000/elapsed_time;
                memcpy(&time_start,&time_end,sizeof(struct timeval));
                track_frame_count = 0;
            }

            _gst_camera->sample_ready = false;
            _gst_camera->convert_finished = false;
            _gst_camera->track_finished = true;
            _gst_camera->show_finished = false;
            _gst_camera->gst_pull_continue();
#endif
        }
        DEBUG_FUNC();
    }
}

static void thread_get_frame(void) 
{
    // for calc get frame rate
    struct timeval  time_start,nframerate_time_end;
	int sample_frame_count=0;
    gettimeofday(&time_start, nullptr);

    DEBUG_FUNC();
    while (true) {
        if(true == _listGstCam.empty()) {
            return ;
        }

        DEBUG_FUNC();
        for(int i  = 0; i < _listGstCam.size(); i++) {
            
            std::shared_ptr<GstSample> sample;
            GstCamera* pCam = _listGstCam.at(i);
            if(pCam->sample_ready != true) {
                pCam->gst_pull_block();
                // continue;
            }
            if(true == pCam->stream_end) {
                continue;
            }

            DEBUG_FUNC();
            pCam->CaptureNoWait(sample);
            if(NULL == sample || NULL == sample.get()) {
                continue;
            }
 
            DEBUG_FUNC();
            GstCaps *smaple_caps = gst_sample_get_caps(sample.get());
            std::shared_ptr<cv::Mat> object_frame;
            gint smaple_width,smaple_height;

            GstStructure *structure = gst_caps_get_structure(smaple_caps,0);
            gst_structure_get_int(structure,"width",&smaple_width);
            gst_structure_get_int(structure,"height",&smaple_height);
            DEBUG_FUNC();
        
            GstBuffer *buffer = gst_sample_get_buffer(sample.get());
            if (NULL == buffer || !smaple_width || !smaple_height) {
                continue;
            }
            cout <<"==sample width="<< smaple_width <<",sample height = " << smaple_height <<  endl;
            GstMapInfo smaple_map;
            gst_buffer_map(buffer,&smaple_map,GST_MAP_READ);

            // Convert gstreamer data to OpenCV Mat
            // still exixst some risk , maybe need corvert style ex.NV12 to RGBA,or other
            cv::Mat tmp_mat;
            if(pCam->is_hwdec()) {
                cout << "enter hardware decoder" << endl;
                unsigned char rgb24[smaple_width * smaple_height*3];
                unsigned char *ybase = (unsigned char *)smaple_map.data;
                unsigned char *vubase = &smaple_map.data[smaple_width * smaple_height];
                //NV12 convert RGB24
                libyuv::NV12ToRGB24(ybase, smaple_width, vubase, smaple_width, rgb24, smaple_width*3, smaple_width, smaple_height);
                std::cout << __FILE__ << "==finished nv12 convert to rgb24==" << __LINE__ << std::endl;
                // get video frame 
                tmp_mat = cv::Mat(smaple_height, smaple_width, CV_8UC3, (unsigned char *)rgb24, cv::Mat::AUTO_STEP);
            } else {
                cout << "enter software decoder" << endl;
                unsigned char rgb24[smaple_width * smaple_height*3];
                unsigned char *ybase = (unsigned char *)smaple_map.data;
                unsigned char *ubase = &smaple_map.data[smaple_width * smaple_height];
                unsigned char *vbase = &smaple_map.data[smaple_width * smaple_height * 5 / 4];
                //YUV420P convert RGB24
                libyuv::I420ToRGB24(ybase, smaple_width, ubase, smaple_width / 2, vbase, smaple_width / 2,rgb24,smaple_width * 3, smaple_width, smaple_height);
                tmp_mat = cv::Mat(smaple_height, smaple_width, CV_8UC3, (unsigned char *)rgb24, cv::Mat::AUTO_STEP);
            }
            //Avoiding memory leaks
            gst_buffer_unmap(buffer, &smaple_map);
            // Make sure the frame rate is close to the true value
#if DEBUG_INFO
            sample_frame_count++; 
            gettimeofday(&nframerate_time_end, nullptr);
            elapsed_time = (nframerate_time_end.tv_sec - time_start.tv_sec) * 1000 +
                (nframerate_time_end.tv_usec - time_start.tv_usec) / 1000;
            if(elapsed_time > 1000*10) {
                printf("[show] showframerate=%f\n", sample_frame_count*1000/elapsed_time);

                // record the frame rate of getting frame
                pCam->set_framerate((int) sample_frame_count * 1000/elapsed_time);

                memcpy(&time_start,&nframerate_time_end,sizeof(struct timeval));
                sample_frame_count = 0;
            }
#endif
            object_frame =  std::make_shared<cv::Mat>(tmp_mat);
            pCam->rgb_object_frame->feed(object_frame);
            cout << "tmp mat width = " << tmp_mat.size().width <<  "  height = " << tmp_mat.size().height << endl;
            cout << "style convert finished" << "tmp_mat != NULL is" << !tmp_mat.empty() << endl;

            pCam->sample_ready = false;
            pCam->convert_finished = true;
            pCam->track_finished = false;
            pCam->show_finished = false;
            pCam->gst_pull_continue();
        }
        DEBUG_FUNC();
    }
}

int main(int argc, char **argv)
{
    cout << __FILE__ << __LINE__ << endl;
    // if(setCameraEnv()) {
    //     printf("camera env init failed\n");
    //     return -1;
    // }
    // system("source /etc/gstreamer1.0/set_gst_env.sh");

    setenv("MALLOC_TRACE", "mtrace.log", 1);
    mtrace();

    int count = 2;
    char config_file[1024] = "/home/user/Desktop/C610-objectTracker-demo-2.0/github/objectTracker-2.0/res/config.ini";

    MulitGstCamera::GstEnvInit();
    GMainLoop *main_loop = g_main_loop_new(NULL,false);
    StreamConf gstCameraConf;
    DEBUG_FUNC();
	for(int i=0; i< count; i++)
	{
		memset(&gstCameraConf, 0, sizeof(StreamConf));
		sprintf(gstCameraConf.gst_dic, "gst_%d",i);
		gst_param_load(config_file,&gstCameraConf);
        DEBUG_FUNC();

        GstCamera *my_gst_camera = new GstCamera();
        DEBUG_FUNC();
        my_gst_camera->set_pipe_name(gstCameraConf.gst_name);
        my_gst_camera->set_sink_name(gstCameraConf.gst_sink);
        my_gst_camera->set_gst_type(gstCameraConf.gst_type);
        my_gst_camera->set_ai_type(gstCameraConf.ai_type);
        my_gst_camera->set_width(gstCameraConf.width);
        my_gst_camera->set_height(gstCameraConf.height);
        my_gst_camera->set_decode_type(gstCameraConf.decode);
        my_gst_camera->set_format(gstCameraConf.format);
        my_gst_camera->set_path(gstCameraConf.path);

        my_gst_camera->set_hw_dec(false);
        my_gst_camera->rgb_object_frame = std::make_shared<BufManager<cv::Mat> > ();
    
        if(AIType::TARGETTRACK == my_gst_camera->get_ai_type()) {

            DEBUG_FUNC();
            std::shared_ptr<cv::Mat> object_frame;
            my_gst_camera->Init(MulitGstCamera::GetPipeline());
            std::cout << my_gst_camera->get_pipeline_str() << std::endl;
            _listGstCam.push_back(my_gst_camera);
        } else if (AIType::OBJECTDETECT == my_gst_camera->get_ai_type()) {
            //waitting compact
            DEBUG_FUNC();
        } else if (AIType::OBJECTDETECT == my_gst_camera->get_ai_type()){
            //waitting compact
            DEBUG_FUNC();
        } else {
            cout << "Disable AI function" << endl;
            my_gst_camera->Init(MulitGstCamera::GetPipeline());
            std::cout << my_gst_camera->get_pipeline_str() << std::endl;
            _listGstCam.push_back(my_gst_camera);
        }
    }
    DEBUG_FUNC();

    std::thread handleThread(thread_get_frame);
    handleThread.detach();

    sleep(2);

// According to the AI type run corresponding thread after enable SUPPORT_AI 
#if SUPPORT_AI
    std::thread trackThread(thread_object_track);
    trackThread.detach();
#endif

#if SHOW_VIDEO
    DEBUG_FUNC();
    {
        std::thread showThread(thread_show);
        showThread.join();
    }
#endif // SHOW_VIDEO
    DEBUG_FUNC();

    g_main_loop_run(main_loop);
    MulitGstCamera::GstEnvDeinit();
    g_main_loop_unref(main_loop);

    return 0;
}