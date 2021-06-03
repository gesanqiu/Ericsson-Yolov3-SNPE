//
// Created by liqing on 18-12-3.
//

#include "YoloClassification.h"
#include "MyUdlLayers.h"
#include "snpe_udl_layers.h"

#include <DlContainer/IDlContainer.hpp>
#include <DlSystem/String.hpp>
#include <DlSystem/DlError.hpp>
#include <DlSystem/ITensor.hpp>
#include <DlSystem/ITensorFactory.hpp>
#include <DlSystem/ITensorFactory.hpp>
#include <DlSystem/IUserBufferFactory.hpp>
#include <DlSystem/TensorShape.hpp>
#include <SNPE/SNPEFactory.hpp>
#include <SNPE/SNPEBuilder.hpp>


#include "TimeUtil.h"

//#define USE_ITENSOR
#define USE_MODEL_FILE

#ifdef USE_MODEL_FILE
//#define MODEL_PATH "/sdcard/Download/yolov3.dlc"
//#define MODEL_PATH "../models/yolov3_0821_quantized_hta.dlc"
#define MODEL_PATH "../models/yolov3-quantize.dlc"

#endif

inline float sigmoid(float x) {
    return (1 / (1 + exp(-x)));
}

/** @brief calculate the rectangle iou */
inline float IoU(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    int xOverlap = std::max(0.0f,
                            std::min(x1 + w1 - 1, x2 + w2 - 1)
                            - std::max(x1, x2) + 1);
    int yOverlap = std::max(0.0f,
                            std::min(y1 + h1 - 1, y2 + h2 - 1)
                            - std::max(y1, y2) + 1);
    int intersection = xOverlap * yOverlap;
    int unio = w1 * h1 + w2 * h2 - intersection;
    return float(intersection) / unio;
}

YoloClassification::YoloClassification():mConfidenceThreshold(0.7f), input_data(NULL),
    outPtr1(NULL), outPtr2(NULL) {
    outShape1 = {MODEL_OUTPUT_BATCH, MODEL_OUTPUT_W/2, MODEL_OUTPUT_H/2, MODEL_OUTPUT_C}; // 13
    outShape2 = {MODEL_OUTPUT_BATCH, MODEL_OUTPUT_W, MODEL_OUTPUT_H, MODEL_OUTPUT_C}; // 26
    outShape3 = {MODEL_OUTPUT_BATCH, MODEL_OUTPUT_W * 2, MODEL_OUTPUT_H * 2, MODEL_OUTPUT_C}; // 52

    anchors.push_back(pairInt(10, 13));
    anchors.push_back(pairInt(16, 30));
    anchors.push_back(pairInt(33, 23));

    anchors.push_back(pairInt(30, 61));
    anchors.push_back(pairInt(62, 45));
    anchors.push_back(pairInt(59, 119));

    anchors.push_back(pairInt(116, 90));
    anchors.push_back(pairInt(156, 198));
    anchors.push_back(pairInt(373, 326));
    std::vector<int> mask1 = {6, 7, 8};
    std::vector<int> mask2 = {3, 4, 5};
    std::vector<int> mask3 = {0, 1, 2};
    anchor_mask.push_back(mask1);
    anchor_mask.push_back(mask2);
    anchor_mask.push_back(mask3);

    for (int j=0; j<MODEL_OUTPUT_W/2; j++) {
        for (int k=0; k<MODEL_OUTPUT_H/2; k++) {
            grid_xy1[j][k][0] = k;
            grid_xy1[j][k][1] = j;
        }
    }
    for (int j=0; j<MODEL_OUTPUT_W; j++) {
        for (int k=0; k<MODEL_OUTPUT_H; k++) {
            grid_xy2[j][k][0] = k;
            grid_xy2[j][k][1] = j;
        }
    }
    for (int j=0; j<MODEL_OUTPUT_W*2; j++) {
        for (int k=0; k<MODEL_OUTPUT_H*2; k++) {
            grid_xy3[j][k][0] = k;
            grid_xy3[j][k][1] = j;
        }
    }
}

YoloClassification::~YoloClassification() {
    deInit();
}

state_t YoloClassification::init(int device) {
    std::unique_ptr<zdl::DlContainer::IDlContainer> container;
#ifdef USE_MODEL_FILE
    container = zdl::DlContainer::IDlContainer::open(zdl::DlSystem::String(MODEL_PATH));
#else
    const std::vector<uint8_t> buffer(&_np_detect_buf[0], &_np_detect_buf[0] + sizeof(_np_detect_buf));
    container = zdl::DlContainer::IDlContainer::open(buffer);
#endif
    zdl::SNPE::SNPEBuilder snpeBuilder(container.get());
#if 1
//    outputLayers.append("conv2d_10/convolution");
//    outputLayers.append("conv2d_13/convolution");
    outputLayers.append("yolov3convolutional59/Conv2D"); // 1x13x13x255
    outputLayers.append("yolov3convolutional67/Conv2D"); // 1x26x26x255
    outputLayers.append("yolov3convolutional75/Conv2D"); // 1x52x52x255
#else
    outputLayers.append("BoxPredictor_0/Reshape:0");
    outputLayers.append("BoxPredictor_0/Reshape_1:0");
    outputLayers.append("BoxPredictor_1/Reshape:0");
    outputLayers.append("BoxPredictor_1/Reshape_1:0");
    outputLayers.append("BoxPredictor_2/Reshape:0");
    outputLayers.append("BoxPredictor_2/Reshape_1:0");
    outputLayers.append("BoxPredictor_3/Reshape:0");
    outputLayers.append("BoxPredictor_3/Reshape_1:0");
    outputLayers.append("BoxPredictor_4/Reshape:0");
    outputLayers.append("BoxPredictor_4/Reshape_1:0");
    outputLayers.append("BoxPredictor_5/Reshape:0");
    outputLayers.append("BoxPredictor_5/Reshape_1:0");
#endif
    zdl::DlSystem::Runtime_t runtime;
    switch (device) {
        case CPU: runtime = zdl::DlSystem::Runtime_t::CPU;
            std::cout<<"CPU"<<std::endl;
            break;
        case GPU: runtime = zdl::DlSystem::Runtime_t::GPU;
            std::cout<<"GPU"<<std::endl;
            break;
        case GPU_16: runtime = zdl::DlSystem::Runtime_t::GPU_FLOAT16;
            std::cout<<"GPU_16"<<std::endl;
            break;
        case DSP: runtime = zdl::DlSystem::Runtime_t::DSP;
            std::cout<<"DSP"<<std::endl;
            break;
        case AIP: runtime = zdl::DlSystem::Runtime_t::AIP_FIXED8_TF;
            std::cout<<"AIP"<<std::endl;
            d_device_id = 4;
            break;
        default:  runtime = zdl::DlSystem::Runtime_t::GPU;break;
    }

//    zdl::DlSystem::UDLBundle udlBundle;
//    udlBundle.cookie = (void*) 0xdeadbeaf;
//    udlBundle.func = MyUDLFactory;
    zdl::DlSystem::PerformanceProfile_t profile = zdl::DlSystem::PerformanceProfile_t::BURST;

#ifdef USE_ITENSOR
    snpe = snpeBuilder.setOutputLayers(outputLayers)
            .setRuntimeProcessor(runtime)
            .setCPUFallbackMode(true)
            .setPerformanceProfile(profile)
          //.setUdlBundle(udlBundle)
          //.setInputDimensions(inputDimensionsMap)
            .build();
#else
    snpe = snpeBuilder.setOutputLayers(outputLayers)
            .setRuntimeProcessor(runtime)
          //  .setCPUFallbackMode(true)
            .setPerformanceProfile(profile)
          //.setUdlBundle(udlBundle)
            .setUseUserSuppliedBuffers(true)
            //.setInputDimensions(inputDimensionsMap)
            .build();
#endif
    if(NULL == snpe) {
        const char* const err = zdl::DlSystem::getLastErrorString();
        std::cout<<"error code"<<std::endl;
    } else {
        const auto strList = snpe->getInputTensorNames();
        auto inputDims = snpe->getInputDimensions((*strList).at(0));
        const zdl::DlSystem::TensorShape& inputShape = *inputDims;
        size_t rank = inputShape.rank();
        input_shape.clear();
        int input_size = 1;
        for (size_t i=0; i<rank; i++) {
            input_size *= inputShape[i];
            input_shape.push_back(inputShape[i]);
            std::cout<<"input shape"<<i<<" "<<inputShape[i]<<std::endl;
        }

        inTensor = zdl::SNPE::SNPEFactory::getTensorFactory().createTensor(
                inputDims
        );
        inMap.add((*strList).at(0), inTensor.get());

        userBuf = createUserBuffer(inputShape, input_data);
        outBuf1 = createUserBuffer(outShape1, outPtr1);
        int out1_size = outShape1[0] * outShape1[1] * outShape1[2] * outShape1[3];
        outTensor1 = zdl::SNPE::SNPEFactory::getTensorFactory().createTensor(
                outShape1, (unsigned char *) outPtr1, out1_size * sizeof(float));

        outBuf2 = createUserBuffer(outShape2, outPtr2);
        int out2_size = outShape2[0] * outShape2[1] * outShape2[2] * outShape2[3];
        outTensor2 = zdl::SNPE::SNPEFactory::getTensorFactory().createTensor(
                outShape2, (unsigned char *) outPtr2, out2_size * sizeof(float));

        int out3_size = outShape3[0] * outShape3[1] * outShape3[2] * outShape3[3];
        outBuf3 = createUserBuffer(outShape3, outPtr3);
        outTensor3 = zdl::SNPE::SNPEFactory::getTensorFactory().createTensor(
                outShape3, (unsigned char *) outPtr3, out3_size * sizeof(float));

        inUBMap.add((*strList).at(0), userBuf.get());
        outUBMap.add("yolov3convolutional59/BiasAdd:0", outBuf1.get()); // 1x13x13x255
        outUBMap.add("yolov3convolutional67/BiasAdd:0", outBuf2.get()); // 1x26x26x255
        outUBMap.add("yolov3convolutional75/BiasAdd:0", outBuf3.get()); // 1x52x52x255

        auto outTensorNames = snpe->getOutputTensorNames();
        std::string name = "";
        for (size_t i = 0; i < (*outTensorNames).size(); i++) {
            name = (*outTensorNames).at(i);
           std::cout<<"output node"<<name.c_str()<<std::endl;
        }

        zdl::DlSystem::Version_t Version = zdl::SNPE::SNPEFactory::getLibraryVersion();
        std::cout<<"version:"<<Version.toString().c_str()<<std::endl;
    }
    return NO_ERROR;
}

state_t YoloClassification::deInit() {
    if (nullptr != snpe) {
        snpe.reset(nullptr);
    }
    if (input_data != NULL) {
        delete[] input_data;
        input_data = nullptr;
    }
    if (outPtr1 != nullptr) {
        delete[] outPtr1;
        outPtr1 = nullptr;
    }
    if (outPtr2 != nullptr) {
        delete[] outPtr2;
        outPtr2 = nullptr;
    }
    return NO_ERROR;
}

void YoloClassification::setConfidence(float value) {
    mConfidenceThreshold = value > 1.0f ? 1.0f : (value < 0.0f ? 0.0f : value);
}

std::vector<S_OBJECT_DATA> YoloClassification::doDetect(std::shared_ptr<cv::Mat> &img) {
    std::vector<S_OBJECT_DATA> result;
    if (NULL == snpe) {
        std::cout<<"Can not init."<<std::endl;
        return result;
    }
    cv::Mat *rgb_mat = img.get();
#if 0
    img = cv::imread("/sdcard/Download/person.jpg");
    cv::cvtColor(img, rgb_mat, CV_BGR2RGB);  // real is bgra to bgr
#else
    //cv::cvtColor(img, rgb_mat, CV_BGRA2RGB);  // real is bgra to bgr
//    cv::imwrite("/sdcard/Download/dump.jpg", img);
#endif
    long pre_start = getCurrentTime_ms();
    int img_w = rgb_mat->cols;
    int img_h = rgb_mat->rows;
    float scale = std::min(MODEL_INPUT_W/(float)img_w, MODEL_INPUT_H/(float)img_h);
    int new_w = img_w * scale;
    int new_h = img_h * scale;
//    long convert_rgb = getCurrentTime_ms();
//    long con_time = getCurrentTime_ms();
//    TS_LOGD("convert rgb cost time=%ld", con_time - convert_rgb);

    int x_offset = (MODEL_INPUT_W - new_w) / 2;
    int y_offset = (MODEL_INPUT_H - new_h) / 2;
    cv::Mat in_mat(MODEL_INPUT_H, MODEL_INPUT_W, CV_8UC3, cv::Scalar(128, 128, 128));
    cv::Mat roi_mat(in_mat, cv::Rect(x_offset, y_offset, new_w, new_h));
    cv::resize(*rgb_mat, roi_mat, cv::Size(new_w, new_h), cv::INTER_CUBIC);
    
    
    cv::Mat input(MODEL_INPUT_H, MODEL_INPUT_W, CV_32FC3, input_data);
    in_mat.convertTo(input, CV_32FC3);
    input /= 255.0f;

#ifdef USE_ITENSOR
    const auto strList = snpe->getInputTensorNames();
    zdl::DlSystem::ITensorItr<false> tensorIt = inTensor->begin();
    zdl::DlSystem::ITensorItr<false> tensorItEnd = inTensor->end();
    int width = input.cols;
    int height = input.rows;
    int channel = input.channels();
    float* ptr = (float*) inTensor.get();
    for (int i=0; i<height; i++) {
        const float *matData = input.ptr<float>(i);
        std::copy(matData, matData + width * channel, tensorIt);
        tensorIt += width * channel;
    }
#endif
    long pre_diff = getCurrentTime_ms() - pre_start;
    ////////std::cout<<"pre process cost time="<<pre_diff<<std::endl;

    long start = getCurrentTime_ms();
    zdl::DlSystem::ITensor* out1 = nullptr;
    zdl::DlSystem::ITensor* out2 = nullptr;
    zdl::DlSystem::ITensor* out3 = nullptr;
#ifdef USE_ITENSOR
    //inMap.add((*strList).at(0), inTensor.get());
    bool ret = snpe->execute(inMap, outMap);
#else
    bool ret = snpe->execute(inUBMap, outUBMap);
#endif
    long diff = getCurrentTime_ms() - start;
    ////////std::cout<<"execute cost time="<<diff<<std::endl;
    if (!ret) {
        const char* const err = zdl::DlSystem::getLastErrorString();
        std::cout<<"error code:"<<err<<std::endl;
        return result;
    }
    uint64_t avg_time = m_time_analyzer.update(diff);
    //////////std::cout<<"LQ-TEST inference time: avg:"<<diff<<avg_time<<std::endl;
    // post process
    long post_start = getCurrentTime_ms();
    int data_size1 = outShape1[0] * outShape1[1] * outShape1[2] * outShape1[3];
    int data_size2 = outShape2[0] * outShape2[1] * outShape2[2] * outShape2[3];
#ifndef USE_ITENSOR
#if 0
    outTensor1 = zdl::SNPE::SNPEFactory::getTensorFactory().createTensor(
            outShape1, (unsigned char *) outPtr1, data_size1 * sizeof(float));
    outTensor2 = zdl::SNPE::SNPEFactory::getTensorFactory().createTensor(
            outShape1, (unsigned char *) outPtr2, data_size2 * sizeof(float));
#else
    //std::copy(outPtr1, outPtr1 + data_size1 * sizeof(4), outTensor1->begin());
    //std::copy(outPtr2, outPtr2 + data_size2 * sizeof(4), outTensor2->begin());
#endif
    out1 = &(*outTensor1);
    out2 = &(*outTensor2);
    out3 = &(*outTensor3);
#else
    out1 = outMap.getTensor("yolov3/yolov3_head/Conv_6/BiasAdd:0"); // 13
    out2 = outMap.getTensor("yolov3/yolov3_head/Conv_14/BiasAdd:0"); // 26
    out3 = outMap.getTensor("yolov3/yolov3_head/Conv_22/BiasAdd:0");  // 52
#endif
    zdl::DlSystem::TensorShape out1_shape = out1->getShape();
    zdl::DlSystem::TensorShape out2_shape = out2->getShape();
    zdl::DlSystem::TensorShape out3_shape = out3->getShape();
    std::vector<int> out1_shape_vec;
    std::vector<int> out2_shape_vec;
    std::vector<int> out3_shape_vec;
    for (int i=0; i<out1_shape.rank(); i++) {
        out1_shape_vec.push_back(out1_shape[i]);
    }
    for (int i=0; i<out2_shape.rank(); i++) {
        out2_shape_vec.push_back(out2_shape[i]);
    }
    for (int i=0; i<out3_shape.rank(); i++) {
        out3_shape_vec.push_back(out3_shape[i]);
    }
    std::vector<zdl::DlSystem::ITensor*> outputTensors;
    outputTensors.push_back(out1);
    outputTensors.push_back(out2);
    outputTensors.push_back(out3);

    float scale_x = MODEL_INPUT_W / (float)img_w;
    float scale_y = MODEL_INPUT_H / (float)img_h;
    float scale_min = scale_x < scale_y ? scale_x : scale_y;
    int new_w1 = round(img_w * scale_min);
    int new_h1 = round(img_h * scale_min);
    float off_x = ((MODEL_INPUT_W - new_w1) / 2.0) / MODEL_INPUT_W;
    float off_y = ((MODEL_INPUT_H - new_h1) / 2.0) / MODEL_INPUT_H;
    float new_scale_x = MODEL_INPUT_W / (float)new_w1;
    float new_scale_y = MODEL_INPUT_H / (float)new_h1;
    std::vector<vec5i> select_indexs_vec2;
    std::vector<vec4f> select_boxes_vec2;
    std::vector<float> select_score_vec2;
    std::vector<int> select_class_vec2;
    for (int i=0; i<3; i++) {
        zdl::DlSystem::ITensor* tensor = outputTensors.at(i);
        zdl::DlSystem::TensorShape shape = tensor->getShape();
        int grid_size[2] = {(int)shape[1], (int)shape[2]};
        std::vector<int>& cur_mask = anchor_mask.at(i);
        std::vector<pairInt> cur_anchors;
        for (int j=0; j<cur_mask.size(); j++) {
            cur_anchors.push_back(anchors.at(cur_mask.at(j)));
        }

        const float* data_ptr = &*(tensor->cbegin());
#ifndef USE_ITENSOR
        // TODO work worng
        if (i==0) {
            data_ptr = outPtr1;
        } else if (i == 1) {
            data_ptr = outPtr2;
        } else if (i == 2) {
            data_ptr = outPtr3;
        }
#endif
        std::vector<vec4i> select_indexs_vec;
        std::vector<float> select_conf_vec;
        for (int j = 0; j < shape[0]; j++) {  // 1
            for (int k = 0; k < shape[1]; k++) {  // 13 or 26
                for (int l = 0; l < shape[2]; l++) {  // 13 or 26
                    for (int m = 0; m < shape[3]; m++) {  // 255
                        float value = *data_ptr++;
                        int n1 = m % 85;
                        int n2 = m / 85;
                        if (n1==4) {
                            float sig = sigmoid(value);
                            if (sig > 0.001) {
                                vec4i idx;
                                idx.v[0] = j;
                                idx.v[1] = k;
                                idx.v[2] = l;
                                idx.v[3] = m;
                                select_indexs_vec.push_back(idx);
                                select_conf_vec.push_back(sig);
                            }
                        }
                    }
                }
            }
        }
        data_ptr = &*(tensor->cbegin());
#ifndef USE_ITENSOR
        // TODO work worng
        if (i == 0) {
            data_ptr = outPtr1;
        } else if (i == 1) {
            data_ptr = outPtr2;
        } else if (i == 2) {
            data_ptr = outPtr3;
        }
#endif
        for (int j=0; j<select_conf_vec.size(); j++) {
            float cur_conf = select_conf_vec.at(j);
            vec4i& idxs = select_indexs_vec.at(j);
            int i_b = idxs.v[0];
            int i_w = idxs.v[1];
            int i_h = idxs.v[2];
            int i_c = idxs.v[3];
            for (int k=0; k<80; k++) {
                int prob_index = ((i_b * shape[1] + i_w) * shape[2] + i_h) * shape[3] + i_c + 1;
                float cur_prob = data_ptr[prob_index + k];
                float cur_prob_sig = sigmoid(data_ptr[prob_index + k]);
                float score = cur_prob_sig * cur_conf;
                if (score > 0.01) {
                    vec5i v5i;
                    v5i.v[0] = i;
                    v5i.v[1] = i_b;
                    v5i.v[2] = i_w;
                    v5i.v[3] = i_h;
                    v5i.v[4] = i_c;
                    select_indexs_vec2.push_back(v5i);
                    select_score_vec2.push_back(score);
                    select_class_vec2.push_back(k);

                    int conf_index = ((i_b * shape[1] + i_w) * shape[2] + i_h) * shape[3] + i_c;
                    float x_val = data_ptr[conf_index - 4];  // x
                    float y_val = data_ptr[conf_index - 3];  // y
                    float w_val = data_ptr[conf_index - 2];  // w
                    float h_val = data_ptr[conf_index - 1];  // h
                    float x_grid_val = 0.0f;
                    float y_grid_val = 0.0f;
                    if (i == 0) {
                        x_grid_val = grid_xy1[i_w][i_h][((i_c-4)%85)%2];
                        y_grid_val = grid_xy1[i_w][i_h][((i_c-3)%85)%2];
                    } else if (i == 1) {
                        x_grid_val = grid_xy2[i_w][i_h][((i_c-4)%85)%2];
                        y_grid_val = grid_xy2[i_w][i_h][((i_c-3)%85)%2];
                    } else if (i == 3) {
                        x_grid_val = grid_xy3[i_w][i_h][((i_c-4)%85)%2];
                        y_grid_val = grid_xy3[i_w][i_h][((i_c-3)%85)%2];
                    }

                    vec4f box;
                    box.v[0] = (sigmoid(x_val) + x_grid_val)/grid_size[1]; // x
                    box.v[1] = (sigmoid(y_val) + y_grid_val)/grid_size[1]; // y
                    int n2 = i_c / 85;
                    pairInt pair = cur_anchors.at(n2);
                    int w_anchor_val = (((i_c - 2)%85)%2 == 0) ? pair.x : pair.y;
                    int h_anchor_val = (((i_c - 1)%85)%2 == 0) ? pair.x : pair.y;
                    box.v[2] = (exp(w_val) * w_anchor_val) / MODEL_INPUT_W;
                    box.v[3] = (exp(h_val) * h_anchor_val) / MODEL_INPUT_H;

                    box.v[0] = (box.v[0] - off_x) * new_scale_x;
                    box.v[1] = (box.v[1] - off_y) * new_scale_y;
                    box.v[2] = box.v[2] * new_scale_x;
                    box.v[3] = box.v[3] * new_scale_y;

                    float min_x = box.v[0] - box.v[2] / 2.0; // min x
                    float min_y = box.v[1] - box.v[3] / 2.0; // min y
                    float max_x = box.v[0] + box.v[2] / 2.0; // max x
                    float max_y = box.v[1] + box.v[3] / 2.0; // max y

                    box.v[0] = min_x * img_w;
                    box.v[1] = min_y * img_h;
                    box.v[2] = max_x * img_w;
                    box.v[3] = max_y * img_h;
                    select_boxes_vec2.push_back(box);
                }
            }
        }
    }
#ifdef USE_ITENSOR
    //inMap.clear();
    outMap.clear();
#else

#endif

    // nms_index = tf.image.non_max_suppression(class_bboxes, class_box_scores, max_bboxes_tensor, iou_threshold=0.5)
    std::vector<int> sort_box_scores_idxs;
    int score_len = select_score_vec2.size();
    for (int j = 0; j < score_len; j++) {
        sort_box_scores_idxs.push_back(j);
    }
    //printf("%s:%d\n", __func__, __LINE__);
    // sort score index
    for (int j = 0; j < score_len; j++) {
        for (int k = score_len - 1 - j; k > 0; k--) {
            int idx_k = sort_box_scores_idxs.at(k);
            int idx_k1 = sort_box_scores_idxs.at(k-1);
            float k_val = select_score_vec2.at(idx_k);
            float k1_val = select_score_vec2.at(idx_k1);
            if (k_val > k1_val) {
                int idx = sort_box_scores_idxs.at(k - 1);
                sort_box_scores_idxs.at(k - 1) = sort_box_scores_idxs.at(k);
                sort_box_scores_idxs.at(k) = idx;
            }
        }
    }

    // select boxes if iou > 0.5
    std::vector<int> select_bbox_idxs;
    for (int j = 0; j < sort_box_scores_idxs.size(); j++) {
        bool isInsert = true;
        int idx = sort_box_scores_idxs.at(j);

        // 取box
        vec4f& box1 = select_boxes_vec2.at(idx);
        float score = select_score_vec2.at(idx);
        float b1_y1 = box1.v[0];
        float b1_x1 = box1.v[1];
        float b1_y2 = box1.v[2];
        float b1_x2 = box1.v[3];
        for (int k = 0; k < select_bbox_idxs.size(); k++) {
            int cur_select_idx = select_bbox_idxs.at(k);
            vec4f &box2 = select_boxes_vec2.at(cur_select_idx);
            float b2_y1 = box2.v[0];
            float b2_x1 = box2.v[1];
            float b2_y2 = box2.v[2];
            float b2_x2 = box2.v[3];
            float iou = IoU(b1_x1, b1_y1, b1_x2 - b1_x1, b1_y2 - b1_y1, b2_x1, b2_y1, b2_x2 - b2_x1, b2_y2 - b2_y1);
            if (iou > 0.5 || select_bbox_idxs.size() > 50) {
                isInsert = false;
            }
        }
        if (isInsert) {
            select_bbox_idxs.push_back(idx);
        }
    }

    // print output boxs
    for (int j = 0; j < select_bbox_idxs.size(); j++) {
        int idx = select_bbox_idxs.at(j);
        float score = select_score_vec2.at(idx);
        if (score > mConfidenceThreshold) {
            S_OBJECT_DATA data;
            vec4f &box = select_boxes_vec2.at(idx);
            int x1 = (int) box.v[0];
            int y1 = (int) box.v[1];
            int x2 = (int) box.v[2];
            int y2 = (int) box.v[3];
            data.x = x1;
            data.y = y1;
            data.width = x2 - x1;
            data.height = y2 - y1;
            data.score = select_score_vec2.at(idx);
            data.label = select_class_vec2.at(idx);
            result.push_back(data);
//             cv::rectangle(img, cv::Rect(x1, y1, x2 - x1, y2 - y1), cv::Scalar(0, 200, 0));
//            LOGD("[%.5f,%.5f,%.5f,%.5f] conf=%.5f label=%d",
//                 box.v[0],box.v[1],box.v[2],box.v[3], select_score_vec2.at(idx), select_class_vec2.at(idx));
        }
    }
//    TS_LOGD("%s:%d", __FUNCTION__, __LINE__);
//     cv::imwrite("/sdcard/Download/temp.jpg", img);
    long post_diff = getCurrentTime_ms() - post_start;
    long sum_diff = getCurrentTime_ms() - pre_start;
    ////////std::cout<<"post cost time="<<post_diff<<std::endl;
    ////////std::cout<<"one frame cost time="<<sum_diff<<std::endl;
    return result;
}

std::unique_ptr<zdl::DlSystem::IUserBuffer> YoloClassification::createUserBuffer(
        zdl::DlSystem::TensorShape shape, float*& data) {
    int buf_size = shape[0] * shape[1] * shape[2] * shape[3];
    std::vector<size_t> strides(shape.rank());
    strides[strides.size() - 1] = sizeof(float);
    size_t stride = strides[strides.size() - 1];
    for (size_t i = shape.rank() - 1; i > 0; i--) {
        stride *= shape[i];
        strides[i - 1] = stride;
    }

    int bufSize = buf_size * sizeof(float);
    float* buffer = new float[buf_size];
    data = (float*) buffer;
    // user buffer
    zdl::DlSystem::UserBufferEncodingFloat userBufferEncodingFloat;
    // create SNPE user buffer from the user-backed buffer
    zdl::DlSystem::IUserBufferFactory &ubFactory = zdl::SNPE::SNPEFactory::getUserBufferFactory();
    return ubFactory.createUserBuffer(data, bufSize, strides,
                                      &userBufferEncodingFloat);
}

