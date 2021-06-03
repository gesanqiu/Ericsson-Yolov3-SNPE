//
// Created by terrytian on 19-3-25.
//

#ifndef FACENNANDROID_TIME_UTILITY_HPP
#define FACENNANDROID_TIME_UTILITY_HPP

#include <cstdint>
#include <list>

namespace ts {

enum class TS_TIME : char {
    TIME_S  = 0,
    TIME_MS = 1,
    TIME_US = 2
};

class TimeUtil {
public:
    explicit TimeUtil(){};

    virtual ~TimeUtil(){};

    static uint64_t getCurrentTime(TS_TIME type);
};

#define NOW_S  ts::TimeUtil::getCurrentTime(ts::TS_TIME::TIME_S)
#define NOW_MS ts::TimeUtil::getCurrentTime(ts::TS_TIME::TIME_MS)
#define NOW_US ts::TimeUtil::getCurrentTime(ts::TS_TIME::TIME_US)

#define TA_DEFAULT_SIZE 100
template <class T>
class TimeAnalyzer {
public:
    TimeAnalyzer() : capacity(TA_DEFAULT_SIZE) {}
    explicit TimeAnalyzer(int size) : capacity(size) {}

    virtual ~TimeAnalyzer(){};

    /**
      * Update the latest time data and return the average of all times
      * @param time : the latest time data/
      * @return : the average of all times in the history list.
      */
    T update(T time) {
        if (mHistoryList.size() > 100) {
            mHistoryList.pop_front();
        }
        mHistoryList.push_back(time);
        double sum = 0;
        for (auto it = mHistoryList.begin(); it != mHistoryList.end(); it++) {
            sum += *it;
        }
        return sum / mHistoryList.size();
    }

private:
    int capacity;
    std::list<T> mHistoryList;
};

}  // namespace ts

#endif  //FACENNANDROID_TIME_UTILITY_HPP
