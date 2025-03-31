/*
 * @Author: Li RF
 * @Date: 2025-03-16 18:12:24
 * @LastEditors: Li RF
 * @LastEditTime: 2025-03-22 12:44:23
 * @Description: 
 * Email: 1125962926@qq.com
 * Copyright (c) 2025 Li RF, All Rights Reserved.
 */
#ifndef OPENCVREADER_H
#define OPENCVREADER_H

#include "Reader.hpp"
#include <iostream>
#include <opencv2/opencv.hpp>

/**
 * @Description: Opencv 引擎
 * @return {*}
 */
class OpencvReader : public Reader {
public:
    OpencvReader();
    ~OpencvReader() override;

    void openVideo(const std::string& filePath) override;
    bool readFrame(cv::Mat& frame) override;
    void closeVideo() override;

private:
    cv::VideoCapture videoCapture; // OpenCV 视频捕获对象
};

#endif // OPENCVREADER_H
        