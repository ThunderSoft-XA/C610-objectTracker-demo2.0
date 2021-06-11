
#include "objecttracker.h"

#include <cstring>
#include <cstdlib>
#include <sys/time.h>
#include "DlContainer/IDlContainer.hpp"
#include "DlSystem/String.hpp"
#include "DlSystem/DlError.hpp"
#include "DlSystem/ITensor.hpp"
#include "DlSystem/ITensorFactory.hpp"
#include "SNPE/SNPEFactory.hpp"
#include "SNPE/SNPEBuilder.hpp"
#include "DlSystem/IUDL.hpp"
#include "DlSystem/UDLContext.hpp"

#define USE_MODEL_FILE
#ifdef USE_MODEL_FILE
// #define GOTURN_MODEL_PATH "/home/user/Desktop/test/objectTracker/models/goturn_quantized.dlc"
 #define GOTURN_MODEL_PATH "/home/user/Desktop/C610-objectTracker-demo-2.0/github/objectTracker-2.0/models/MobileNetSSD_deploy.dlc"
#endif

#define MODEL_INPUT_W 300
#define MODEL_INPUT_H 300
#define MODEL_INPUT_C 3

#include <iostream>
#include <vector>

using namespace zdl;
using namespace objecttracker;
using namespace std;

ObjectTracker::ObjectTracker(int device) 
{
    this->isSiam = false;
    this->needFirstFrame = true;
    this->mConfidenceThreshold = 0.8;
    this->init(device);
}

state_t ObjectTracker::init(int device) {
    std::unique_ptr<zdl::DlContainer::IDlContainer> container;
#ifdef USE_MODEL_FILE
    container = zdl::DlContainer::IDlContainer::open(zdl::DlSystem::String(GOTURN_MODEL_PATH));
#endif
    zdl::SNPE::SNPEBuilder snpeBuilder(container.get());


    zdl::DlSystem::Runtime_t runtime;
    switch (device) {
        case CPU: runtime = zdl::DlSystem::Runtime_t::CPU;break;
        case GPU: runtime = zdl::DlSystem::Runtime_t::GPU;break;
        case DSP: runtime = zdl::DlSystem::Runtime_t::DSP;break;
        case APU: runtime = zdl::DlSystem::Runtime_t::AIP_FIXED8_TF; break;
        default:  runtime = zdl::DlSystem::Runtime_t::GPU;break;
    }

    zdl::DlSystem::UDLBundle udlBundle;
    zdl::DlSystem::PerformanceProfile_t profile = zdl::DlSystem::PerformanceProfile_t::HIGH_PERFORMANCE;

    snpe = snpeBuilder.setOutputLayers(outputLayers)
            .setRuntimeProcessor(runtime)
            .setCPUFallbackMode(true)
            .setPerformanceProfile(profile)
            .setUdlBundle(udlBundle)
            .build();
    if(nullptr == snpe) {
        const char* const err = zdl::DlSystem::getLastErrorString();
        std::cout<<"!!!!!! ERROR code :"<<err<<std::endl;
    } else {
        const auto strList = snpe->getInputTensorNames();
        auto inputDims = snpe->getInputDimensions((*strList).at(0));
        const zdl::DlSystem::TensorShape& inputShape = *inputDims;
        size_t rank = inputShape.rank();
        int input_size = 1;
        for (size_t i=0; i<rank; i++) {
            input_size *= inputShape[i];
#ifdef DEBUG
            std::cout << "input shape" << i << ":" << inputShape[i] <<std::endl;
#endif // DEBUG
        }

        inTensor = zdl::SNPE::SNPEFactory::getTensorFactory().createTensor(
                inputDims
        );
        inMap.add((*strList).at(0), inTensor.get());
        std::cout<<"snpe init finish"<<std::endl;
        zdl::DlSystem::Version_t Version = zdl::SNPE::SNPEFactory::getLibraryVersion();
#ifdef DEBUG
    std::cout << "snpe version == " << Version << std::endl;
#endif // DEBUG
    }      
    return NO_ERROR;
}

state_t ObjectTracker::deInit() {
    if (nullptr != snpe) {
        snpe.reset(nullptr);
    }
    return NO_ERROR;
}

std::vector<unsigned char> ObjectTracker::loadByteDataMat(cv::Mat &_data_mat)
{
    assert(_data_mat.empty() != true);
   std::vector<unsigned char> vec;
   std::cout << __FILE__ << "mat to unsigned char" << __LINE__ << std::endl;
   loadByteDataMat(_data_mat, vec);
   return vec;
}

template<typename T>
bool ObjectTracker::loadByteDataMat(cv::Mat &_input_mat, std::vector<T>& loadVector)
{
    cv::Mat input;
    cv::Size src_size = cv::Size(cv::Size(MODEL_INPUT_W, MODEL_INPUT_H));
    std::cout << "input mat`s channels = " << _input_mat.channels() << std::endl;
    cv::Mat resize_mat = cv::Mat(src_size, _input_mat.channels());
    std::cout << __FILE__ << "unsigned char mat processing..." << __LINE__ << std::endl;
    cv::resize(_input_mat, resize_mat, src_size);
    cv::cvtColor(resize_mat, input, CV_BGR2RGB);

    cv::Mat input_norm(MODEL_INPUT_W, MODEL_INPUT_H, CV_32FC3, inTensor.get()->begin().dataPointer());
    input.convertTo(input, CV_32F);
    cv::normalize(input, input_norm, -1.0f, 1.0f, cv::NORM_MINMAX);

    loadVector = (vector<unsigned char>)(input_norm.reshape(1, 1));
#if 0
    int row = input_norm.rows;
    int col = input_norm.cols;
    std::vector<unsigned char> _vector(row*col);
    for (int i = 0; i < row; i ++){
        for (int j = 0; j < col; j ++){
            _vector.push_back(input_norm.at<unsigned char>(i, j));
        }
    }
    loadVector = _vector;
#endif
    std::cout << "loadByteDataMat to _vector`s size =  " << loadVector.size() << std::endl;
    std::cout << __FILE__ << "unsigned char mat finished..." << __LINE__ << std::endl;
    return true;
}

// Load multiple input tensors for a network which require multiple inputs
std::tuple<zdl::DlSystem::TensorMap, bool> ObjectTracker::loadMultipleInput (std::unique_ptr<zdl::SNPE::SNPE>& snpe , std::vector<cv::Mat>& mat_vec)
{
    assert(mat_vec.empty() != true);
    std::cout << __FILE__ << "=vector mat`s count = "<< mat_vec.size()  << __LINE__ << std::endl;
    zdl::DlSystem::TensorMap dummy; // dummy map for returning on failure
    const auto& inputTensorNamesRef = snpe->getInputTensorNames();
    if (!inputTensorNamesRef) throw std::runtime_error("Error obtaining Input tensor names");
    const auto &inputTensorNames = *inputTensorNamesRef;
    // Make sure the network requires multiple inputs
    assert (inputTensorNames.size() > 1);

    if (inputTensorNames.size()) std::cout << "Processing DNN Input: " << std::endl;

    std::vector<std::unique_ptr<zdl::DlSystem::ITensor>> inputs(inputTensorNames.size());
    zdl::DlSystem::TensorMap  inputTensorMap;
#if 0
    for(auto &_mat : mat_vec) {
        assert(_mat.empty() != true);
        std::cout << __FILE__ << "load multiple input processing..." << __LINE__ << std::endl;
#endif
        std::cout << "input tensor num = " << inputTensorNames.size() << std::endl; 
        for (size_t j = 0; j<inputTensorNames.size(); j++) {
            std::string inputName(inputTensorNames.at(j));
            std::vector<unsigned char> inputVec = loadByteDataMat(mat_vec[j]);

            const auto &inputShape_opt = snpe->getInputDimensions(inputTensorNames.at(j));
            const auto &inputShape = *inputShape_opt;
            inputs[j] = zdl::SNPE::SNPEFactory::getTensorFactory().createTensor(inputShape);

            if (inputs[j]->getSize() != inputVec.size()) {
                std::cerr << "Size of input does not match network.\n"
                          << "Expecting: " << inputs[j]->getSize() << "\n"
                          << "Got: " << inputVec.size() << "\n";
                return std::make_tuple(dummy, false);
            }

            std::copy(inputVec.begin(), inputVec.end(), inputs[j]->begin());
            inputTensorMap.add(inputName.c_str(), inputs[j].release());
        }
#if 0
    }
#endif
    std::cout << __FILE__ << "Finished processing inputs for current inference " << __LINE__ << std::endl;
    return std::make_tuple(inputTensorMap, true);
}

std::vector<float> ObjectTracker::doTracking(std::vector<cv::Mat>& mat_vec) 
{
    assert(mat_vec.empty() != true);
    struct timeval  time_start,time_end;
    gettimeofday(&time_start, nullptr);
    std::vector<float> result;
    if (NULL == snpe) {
        //TS_LOGE("can not init err!");
        return result;
    }
    std::cout << __FILE__ << "do track processing..." << __LINE__ << std::endl;
    std::tuple<zdl::DlSystem::TensorMap, bool> input_tuple = this->loadMultipleInput(this->snpe, mat_vec);
    this->inMap = std::get<0> (input_tuple);

    //long diff_pre = getCurrentTime_ms() - start_pre;
    zdl::DlSystem::ITensor* outputTensor = nullptr;
    //long start = getCurrentTime_ms();
    bool ret = snpe->execute(inMap, outMap);
    //long diff = getCurrentTime_ms() - start;
    if (!ret) {
        const char* const err = zdl::DlSystem::getLastErrorString();
        std::cout << "!!!!!!ERROR code:" << err << std::endl;
        return result;
    }
    zdl::DlSystem::StringList tensorNames = outMap.getTensorNames();
    for( auto& name: tensorNames ){
        outputTensor = outMap.getTensor(name);
        std::cout << "tensor name: "<< name << std::endl;
        std::cout << "OutputShape : " << outputTensor->getShape().getDimensions()[0] << " x "
            << outputTensor->getShape().getDimensions()[1] << std::endl;
    }
    zdl::DlSystem::TensorShape shape = outputTensor->getShape();

    size_t rank = shape.rank();
    int input_size = 1;
    for (size_t i = 0; i < rank; i++) {
        input_size *= shape[i];
        std::cout << "output shape" << i << ":" << shape[i] << std::endl;
    }
    result.clear();
    double elapsed_time;
    gettimeofday(&time_end, nullptr);
    elapsed_time = (time_end.tv_sec - time_start.tv_sec) * 1000 +
        (time_end.tv_usec - time_start.tv_usec) / 1000;
    cout << "the time of track = " << elapsed_time << endl;
    std::cout << " input size = " << input_size << std::endl;
    cout << "Output Rusult : "<<endl;
    for ( auto it = outputTensor->cbegin(); it != outputTensor->cend() ;it++ ) {
        float f = *it;
        cout <<f << " ";
        result.push_back(f);
    }
    cout <<endl;
    outMap.clear();

    std::cout << __FILE__ << "do detect finished..." << __LINE__ << std::endl;
    return result;
}

void ObjectTracker::setConfidence(float value) {
    mConfidenceThreshold = value > 1.0f ? 1.0f : (value < 0.0f ? 0.0f : value);
}


std::vector<float> ObjectTracker::doTracking(cv::Mat object_mat)
{
    std::vector<float> result;
    if (NULL == snpe) {
        return result;
    }
    cv::Mat input;

    // cv resize
#if 1
    cv::Mat resize_mat;
    cv::resize(object_mat, resize_mat, cv::Size(MODEL_INPUT_W, MODEL_INPUT_H));
    cv::cvtColor(resize_mat, input, CV_BGR2RGB);

    cv::Mat input_norm(MODEL_INPUT_W, MODEL_INPUT_H, CV_32FC3, inTensor.get()->begin().dataPointer());
    input.convertTo(input, CV_32F);
    cv::normalize(input, input_norm, -1.0f, 1.0f, cv::NORM_MINMAX);
#endif

    zdl::DlSystem::ITensor* outputTensor = nullptr;
    bool ret = snpe->execute(inMap, outMap);
    if (!ret) {
        const char* const err = zdl::DlSystem::getLastErrorString();
        std::cout << "!!!!!!ERROR code:" << err << std::endl;
        return result;
    }
    zdl::DlSystem::StringList tensorNames = outMap.getTensorNames();
    for( auto& name: tensorNames ){
        std::cout << "tensor name: "<< name << std::endl;
        outputTensor = outMap.getTensor(name);
        std::cout << "OutputShape : " << outputTensor->getShape().getDimensions()[0] << " x "
            << outputTensor->getShape().getDimensions()[1] << " x "
            << outputTensor->getShape().getDimensions()[2] << " x "
            << outputTensor->getShape().getDimensions()[3] << std::endl;
    }
    zdl::DlSystem::TensorShape shape = outputTensor->getShape();

    size_t rank = shape.rank();
    int input_size = 1;
    for (size_t i = 0; i < rank; i++) {
        input_size *= shape[i];
        std::cout << "output shape" << i << ":" << shape[i] << std::endl;
    }
    std::cout << __func__ << __LINE__ << std::endl;

    const float* final_result = &*(outputTensor->cend());

    final_result = final_result - 7;
    for(int i =0; i < shape[3]; i++){
        float confidence = *final_result++;
        result.push_back(confidence);
        std::cout <<"i: "<< i << std::endl;
        std::cout <<"confidence: "<< confidence << std::endl;
    }
    std::cout << __func__ << __LINE__ << std::endl;

    return result;
}

void ObjectTracker::setSiam(bool _isSiam)
{
    this->isSiam = _isSiam;
}

bool ObjectTracker::getSiam()
{
    return this->isSiam;
}

ObjectTracker::~ObjectTracker() {
    if (snpe != nullptr) {
        snpe.reset(nullptr);
    }
}