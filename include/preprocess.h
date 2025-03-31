/*
 * @Author: Li RF
 * @Date: 2024-11-26 10:07:30
 * @LastEditors: Li RF
 * @LastEditTime: 2025-03-27 16:35:22
 * @Description: 
 * Email: 1125962926@qq.com
 * Copyright (c) 2025 Li RF, All Rights Reserved.
 */
#ifndef _RKNN_YOLOV5_DEMO_PREPROCESS_H_
#define _RKNN_YOLOV5_DEMO_PREPROCESS_H_

#include <stdio.h>
#include "im2d.h"
#include "rga.h"
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "postprocess.h"

void letterbox(const cv::Mat &image, cv::Mat &padded_image, BOX_RECT &pads, const float scale, const cv::Size &target_size, bool Use_opencl = true, const cv::Scalar &pad_color = cv::Scalar(128, 128, 128));
int RGA_resize(const cv::Mat &image, cv::Mat &resized_image);
int RGA_handle_resize(const cv::Mat &image, cv::Mat &resized_image);
int RGA_bgr_to_rgb(const cv::Mat& rgb_origin, cv::Mat &bgr_image);
int RGA_handle_bgr_to_rgb(const cv::Mat& rgb_origin, cv::Mat &bgr_image);
int RGA_yuv420sp_to_bgr(const uint8_t *frame_yuv_data, const int& width, const int& height, cv::Mat &bgr_image);
int RGA_handle_yuv420sp_to_bgr(const uint8_t *frame_yuv_data, const int& width, const int& height, cv::Mat &bgr_image);

#endif //_RKNN_YOLOV5_DEMO_PREPROCESS_H_
