/*
 * Copyright © 2018-2019 Thunder Software Technology Co., Ltd.
 * All rights reserved.
 */

#include "time_utility.hpp"

#include <sys/time.h>
#define __USE_XOPEN
#include <time.h>
#include <chrono>

namespace ts {

uint64_t TimeUtil::getCurrentTime(TS_TIME type) {
    struct timeval tv;
    uint64_t time;
#if 0
    gettimeofday(&tv, NULL);
    if (type == TS_TIME::TIME_S) {  // second
        time = tv.tv_sec;
        } else if (type == TS_TIME::TIME_MS) {  // millisecond
            time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        } else {  // microsecond
            time = tv.tv_sec * 1000000 + tv.tv_usec;
    }
#else
    auto time_now = std::chrono::system_clock::now();
    if (type == TS_TIME::TIME_S) {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                time_now.time_since_epoch());
        time = duration.count();
    } else if (type == TS_TIME::TIME_MS) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                time_now.time_since_epoch());
        time = duration.count();
    } else {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                time_now.time_since_epoch());
        time = duration.count();
    }
#endif
    return time;
}

}  // namespace ts

//template<class T>
//TimeAnalyzer<T>::TimeAnalyzer(int size) : capacity(size) {}
//
//template<class T>
//T TimeAnalyzer<T>::update(T time) {
//    if (mHistoryList.size() > 100) {
//        mHistoryList.pop_front();
//    }
//    mHistoryList.push_back(time);
//    double sum = 0;
//    for (auto it = mHistoryList.begin(); it != mHistoryList.end(); it++) {
//        sum += *it;
//    }
//    return sum / mHistoryList.size();
//}
