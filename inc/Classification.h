//
// Created by liqing on 18-12-3.
//

#ifndef AI_DEMO_CLASSIFICATION_H
#define AI_DEMO_CLASSIFICATION_H

#include "jni_types.h"

#include <SNPE/SNPE.hpp>

//#include <opencv/cv.h>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>

class Classification {
public:
    explicit Classification();
    virtual ~Classification();

    state_t init(int device);
    state_t deInit();
    std::vector<S_OBJECT_DATA> doDetect(cv::Mat img);
    void setConfidence(float value);

private:
    std::unique_ptr<zdl::SNPE::SNPE> snpe;
    zdl::DlSystem::StringList outputLayers;
    std::shared_ptr<zdl::DlSystem::ITensor> inTensor;
    zdl::DlSystem::TensorMap outMap;
    zdl::DlSystem::TensorMap inMap;

    float mConfidenceThreshold;
};

#endif //AI_DEMO_CLASSIFICATION_H
