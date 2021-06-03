//
// Created by liqing on 18-8-31.
//

#ifndef _TIMEUTIL_H
#define _TIMEUTIL_H

#include <sys/time.h>

#define __USE_XOPEN
#include <time.h>

inline long getCurrentTime_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

inline long getCurrentTime_s() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

#include <iostream>
#include <sstream>
template <typename T>
inline std::string rdc_int2string(T n) {
    std::stringstream ss;
    std::string str;
    ss << n;
    ss >> str;
    return str;
}

inline std::string getCurrentTimeStr() {
    // get current system time
    time_t now;
    time(&now);
    struct tm* local_tm = localtime(&now);
    std::string year = rdc_int2string(local_tm->tm_year + 1900);
    std::string month = rdc_int2string(local_tm->tm_mon + 1);
    std::string day = rdc_int2string(local_tm->tm_mday);
    std::string hour = rdc_int2string(local_tm->tm_hour);
    std::string minute = rdc_int2string(local_tm->tm_min);
    std::string second = rdc_int2string(local_tm->tm_sec);
    std::string str;
    str.append(year).append("-").append(month).append("-").append(day)
            .append("-").append(hour).append("-").append(minute).append("-")
            .append(second);
    return str;
}

#include <stdlib.h>
#include <ctime>
inline int getRandNum(int num) {
    srand(time(0));
    return (rand() % num);
}

#endif //_TIMEUTIL_H
