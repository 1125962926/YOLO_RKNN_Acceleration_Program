/*
 * @Author: Li RF
 * @Date: 2025-03-16 18:10:16
 * @LastEditors: Li RF
 * @LastEditTime: 2025-03-19 12:16:06
 * @Description: 
 * Email: 1125962926@qq.com
 * Copyright (c) 2025 Li RF, All Rights Reserved.
 */
#ifndef READER_H
#define READER_H

#include <string>
#include "opencv2/core.hpp"

/**
 * @Description: 基类引擎
 * @return {*}
 */
class Reader {
public:
    // 析构虚函数
    virtual ~Reader() = default;
    /* 纯虚函数接口 */
    virtual void openVideo(const std::string& filePath) = 0;
    virtual bool readFrame(cv::Mat& frame) = 0;
    virtual void closeVideo() = 0;
};

#endif // READER_H