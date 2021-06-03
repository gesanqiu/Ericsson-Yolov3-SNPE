#ifndef NIKEDEMO_JNI_TYPES_H
#define NIKEDEMO_JNI_TYPES_H

//typedef struct {
//    int rect[4];  /* x, y, width, height */
//    int label;
//    float confidence;
//} CLASSIFY_DATA;

typedef enum {
    EXECUTE_ERROR = -2,
    INVALID_INPUT = -1,
    NO_ERROR = 0
}state_t;

typedef enum  {
    CPU         = 0,
    GPU         = 1,
    GPU_16      = 2,
    DSP         = 3,
    AIP         = 4
}runtime_t;

#include <iostream>
#include <sstream>
template <typename T>
inline std::string int2string(T n) {
    std::stringstream ss;
    std::string str;
    ss << n;
    ss >> str;
    return str;
}

template <typename Dtype>
struct DVec4 {
    union {
        Dtype v[4];
        struct {Dtype x, y, z, w;};
    };
};
typedef DVec4<int> vec4i;
typedef DVec4<float> vec4f;

template <typename Dtype, size_t SIZE>
struct DVec {
    union {
        Dtype v[SIZE];
    };
};
typedef DVec<int, 5> vec5i;

template <typename Dtype>
struct DPair {
    Dtype x; Dtype y;

    DPair() {}
    ~DPair() {}

    DPair(Dtype v1, Dtype v2) {
        x = v1;
        y = v2;
    }
};

typedef DPair<int> pairInt;
typedef DPair<float> pairFloat;
typedef DPair<double> pairDouble;
typedef DPair<std::string> pairStr;

#define LOG_SWITCH 0
#define TS_LOG_TAG ("TS_TAG_")

#endif
