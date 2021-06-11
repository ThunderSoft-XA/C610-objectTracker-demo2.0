#ifndef __OBJECT_TRACKER_H__
#define __OBJECT_TRACKER_H__

//#include "jni_types.h"
//#include "common.h"

#include "SNPE/SNPE.hpp"
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include "common.h"
#include "bufmanager.hpp"

namespace objecttracker{

class ObjectTracker {
    public:
        explicit ObjectTracker(){}
        explicit ObjectTracker(int device);
        virtual ~ObjectTracker();
        std::mutex _gstTimerMut;

        int object_num;

        state_t init(int device);
        state_t deInit();
        void setConfidence(float value);
        void setSiam(bool _isSiam);
        bool getSiam();
        std::vector<float> doTracking(std::vector<cv::Mat>& mat_vec);
        std::vector<float> doTracking(cv::Mat object_mat); 

    private:
        std::unique_ptr<zdl::SNPE::SNPE> snpe;
        zdl::DlSystem::StringList outputLayers;
        std::shared_ptr<zdl::DlSystem::ITensor> inTensor;
        zdl::DlSystem::TensorMap outMap;
        zdl::DlSystem::TensorMap inMap;
        std::unique_ptr<zdl::DlSystem::ITensor> input;
        std::shared_ptr<BufManager<GstSample> > frame_cache;


        bool isSiam;
        bool needFirstFrame;
        float mConfidenceThreshold;

        std::vector<unsigned char> loadByteDataMat(cv::Mat &_data_mat);
        template<typename T> bool loadByteDataMat(cv::Mat &_input_mat, std::vector<T>& loadVector);
        std::tuple<zdl::DlSystem::TensorMap, bool> loadMultipleInput (std::unique_ptr<zdl::SNPE::SNPE>& snpe , std::vector<cv::Mat>& mat_vec);
    };

};

#endif //__OBJECT_DETECTION_H__
