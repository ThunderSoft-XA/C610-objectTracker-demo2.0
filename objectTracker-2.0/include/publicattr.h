#ifndef __PUBLIC_ATTR_H__
#define __PUBLIC_ATTR_H__

#include <iostream>
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include <gst/gst.h>

#ifndef EXPORT_API
#define EXPORT_API __attribute__ ((visibility("default")))
#endif

using namespace std;

namespace gstcamera
{

#define DEBUG_FUNC() std::cout << __FILE__ << "=======" << __LINE__ << std::endl

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  the local camera for C610 camera on board,gstreamer plugins is qti series
 *                 the rtsp for uri stream
 */
typedef enum _GstType {
    LOCALFILE = 0,
    CAMERA,
    RTSP
}GstType;

typedef enum _AIType {
    NONE = 0,
    TARGETTRACK = 1,
    OBJECTDETECT,
    POSE
} AIType;

/**
 * @brief the pipeline`s last output to local file or tcp client or udp client
 *LOCAL_FILE will use filesink or multifilesink
 * TCP will use tcpsink,needing support for tool same like vlc etc...
 * UDP will udp sink
 * SCREEN will directly draw to screen by wayland and gtk
 */
typedef enum _CameraDest {
    LOCAL_FILE = 0,
    TCP,
    UDP,
    SCREEN
} CameraDest;

#ifdef __cplusplus
}
#endif

class TrackInfo
{
public:
    cv::Mat show_mat;
    float confidence;
    string target_name;
    std::vector<cv::Rect2f> track_data;
};

class FrameAttr
{
private:
    /**
     * @brief These data describe a frame info, some member varitiant isn`t necessary
     * 
     */
    int width;
    int height;
    int channels;
    string format;
    unsigned long index;
    std::string path;

public:
    FrameAttr(/* args */){ }
    ~FrameAttr(){ }

    void set_width(int _frame_w) { 
        this->width = _frame_w;
    } 
    int get_width() {
        return this->width;
    }

    void set_height(int _frame_h) {
        this->height = _frame_h;
    }
    int get_height() {
        return height;
    }

    void set_channels(int _frame_c) {
        this->height = _frame_c;
    }
    int get_channels() {
        return this->channels;
    }

    void set_format(string _frame_format)
    {
        #define FORMATED 0
        this->format = _frame_format;
    }
    string get_format()
    {
        return this->format;
    }

    void set_index(unsigned long _frame_index) {
        this->index = _frame_index;
    }
    unsigned long get_index() {
        return this->index;
    }

    void set_path(std::string _frame_path) {
        this->path = _frame_path;
    }
    std::string get_path() {
        return this->path;
    }
};

/**
 * @brief some key info only video
 * 
 */
class VideoAttr : public FrameAttr
{
private:
    /* data */
    string decode_type;
    int framerate;
public:
    VideoAttr(/* args */){ }
    ~VideoAttr(){ }

    void set_decode_type(string _video_d)
    {
        this->decode_type = _video_d;
    }
    string get_decode_type()
    {
        return this->decode_type;
    }

    void set_framerate(int _video_f) {
        this->framerate = _video_f;
    }
    int get_framerate() {
        return this->framerate;
    }

};

#if 0
 /// Base definition for GstElemnet or GstBus etc for factory build...
template <typename T>
struct Gst_T {
    Gst_T() {}
    Gst_T(T* _element,gchar* _name,Order_t _order = 0) {
        this->element = _element;
        this->name = _name;
        this->order = _order;
    }
    T* element;
    /**
     * @brief most of elements don`t need name
     * @type why is gchar * ? avoid meaningless convert
     */
    gchar* name;
    Order_t  order;
};
using GstElementInfo = Gst_T<GstElement>;
using  GstBusInfo = Gst_T<GstBus>;
using GstCapsInfo = Gst_T<GstCaps>;

#endif

} // namespace c610gst

#endif // !__PUBLIC_ATTR_H__