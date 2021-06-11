#ifndef __BUFMANAGER_H__
#define __BUFMANAGER_H__

#include <iostream>
#include <mutex>
#include <semaphore.h>
#include <atomic>
#include <memory>
#include <list>
#include <gst/gst.h>
#include <gst/app/app.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>

#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/types_c.h"


//using namespace std;
//namespace buffer

template<typename T>
class BufManager {
public:
    BufManager();
    ~BufManager();
    void feed(std::shared_ptr<T> pending);
    std::shared_ptr<T> front();
    std::shared_ptr<T> fetch();
private:
    std::atomic<bool> swap_ready;
    // std::mutex swap_mtx;
    sem_t g_semaphore;
    std::shared_ptr<T> front_sp;
    std::shared_ptr<T> back_sp;
public:
    std::string debug_info;

};
//typedef BufManager<cv::Mat> MatBufManager;
//typedef BufManager<GstSample> GstBufManager;
//#include "BufManager.tpp"

/** @brief It provides an easy-to-use, high-level interface to variety FaceSDK Algorithms.
 *
 * */
template<typename T> 
BufManager<T>::BufManager() /*noexcept */ {
	//swap_ready(false);
    sem_init(&g_semaphore, 0 , 1);
	swap_ready = false;
}

template<typename T>
BufManager<T>::~BufManager() /*noexcept*/ {
	if (!debug_info.empty() ) {
		printf("BufManager %s destroyed.", debug_info.c_str());
	}
}
/** @brief Put the latest buffer into cache queue to be processed.
 *
 * Giving up control of previous front buffer.
 * @param[in] The latest buffer.
 * */
template<typename T> 
void BufManager<T>::feed(std::shared_ptr<T> pending) {
	if (nullptr == pending.get()) {
		throw "ERROR: feed an empty buffer to BufManager";
	}
	// swap_mtx.lock();
    sem_wait(&g_semaphore);
	front_sp = pending;
	swap_ready = true;
    sem_post(&g_semaphore);
		// swap_mtx.unlock();
        return;
    }

    /** @brief Get the front buffer.
     * @return Front buffer.
     * */
template<typename T> 
std::shared_ptr<T> BufManager<T>::front()  /*noexcept */{
        return front_sp;
    }

    /** @brief Fetch the shared back buffer.
     * @return Back buffer.
     * */
template<typename T>
std::shared_ptr<T> BufManager<T>::fetch()  /*noexcept */{
        if (swap_ready) {
            // swap_mtx.lock();
            sem_wait(&g_semaphore);
            std::swap(back_sp, front_sp);
			swap_ready = false;
            sem_post(&g_semaphore);
            // swap_mtx.unlock();
        }
        return back_sp;
    }
    
template class BufManager<cv::Mat>;
template class BufManager<GstSample>;

#endif
