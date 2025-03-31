/*
 * @Author: Li RF
 * @Date: 2024-11-26 10:07:30
 * @LastEditors: Li RF
 * @LastEditTime: 2025-03-20 17:39:38
 * @Description: 
 * Email: 1125962926@qq.com
 * Copyright (c) 2024 Li RF, All Rights Reserved.
 */
#ifndef RKYOLOV5S_H
#define RKYOLOV5S_H

#include "rknn_api.h"
#include "opencv2/core/core.hpp"
#include "SharedTypes.hpp"

static void dump_tensor_attr(rknn_tensor_attr *attr);
static unsigned char *load_data(FILE *fp, size_t ofst, size_t sz);
static unsigned char *load_model(const char *filename, int *model_size);

class rkYolo
{
private:
    int ret;
    std::mutex mtx;
    AppConfig config;

    rknn_context ctx;
    rknn_input_output_num io_num;
    // rknn_tensor_attr *input_attrs;
    // rknn_tensor_attr *output_attrs;
    // 更新为智能指针
    std::unique_ptr<rknn_tensor_attr[]> input_attrs;
    std::unique_ptr<rknn_tensor_attr[]> output_attrs;
    rknn_input inputs[1];

    int channel, width, height;

    float nms_threshold, box_conf_threshold;

public:
    rkYolo(const AppConfig& config);
    int init(rknn_context *ctx_in, bool isChild);
    rknn_context *get_pctx();
    cv::Mat infer(cv::Mat ori_img);
    ~rkYolo();
};

#endif