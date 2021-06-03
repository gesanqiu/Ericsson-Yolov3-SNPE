#include <iostream>
#include <vector>
#include <string>
#include <fstream>

#if 0
#include "core.hpp"
#include "opencv.hpp"
#include "imgproc.hpp"
#include "imgproc/imgproc.hpp"
#endif
#if 1
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#endif
#include "YoloClassification.h"
#include "jni_types.h"

extern std::string* labels;

std::vector<S_OBJECT_DATA> yolov3_detection(std::shared_ptr<cv::Mat>& img, YoloClassification* detector) {

    //cv::Mat img = cv::imread("./orange.jpeg");
    //if(img.empty()){
    //    std::cout<<"empty"<<std::endl;
    // }
    //test
    std::vector<S_OBJECT_DATA> result = detector->doDetect(img);
    std::cout<<"data size: "<< result.size()<<std::endl;
    if(result.size()>0){
        for(int i =0;i< result.size();i++){
            std::cout<<"name : "<< labels[result[i].label]<<std::endl;
            std::cout<<"position(x, y, width, height):"<<std::endl;
            std::cout<<"x:"<<result[i].x<<std::endl;
            std::cout<<"y:"<<result[i].y<<std::endl;
            std::cout<<"width:"<<result[i].width<<std::endl;
            std::cout<<"height:"<<result[i].height<<std::endl;
            //std::cout<<"x:"<<result[i].rect[0]<<std::endl;
            //std::cout<<"y:"<<result[i].rect[1]<<std::endl;
            //std::cout<<"width:"<<result[i].rect[2]<<std::endl;
            //std::cout<<"height:"<<result[i].rect[3]<<std::endl;
        }
    }
    return result;
}
