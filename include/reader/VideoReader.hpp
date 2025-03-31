/*
 * @Author: Li RF
 * @Date: 2025-03-15 16:31:57
 * @LastEditors: Li RF
 * @LastEditTime: 2025-03-19 16:59:54
 * @Description: 
 * Email: 1125962926@qq.com
 * Copyright (c) 2025 Li RF, All Rights Reserved.
 */
#ifndef VIDEOREADER_H
#define VIDEOREADER_H

#include <memory>
#include <string>

#include "SharedTypes.hpp"
#include "Reader.hpp"

/**
 * @Description: 视频读取器
 * @return {*}
 */
class VideoReader {
public:
    VideoReader(const AppConfig& config);
    ~VideoReader();

    /* 以下禁止拷贝和允许移动两部分实现：
    1、提高性能；
    2、管理独占资源；
    3、现代C++鼓励使用移动语义和智能指针等工具来管理资源。 
    */
    // 禁止拷贝构造和拷贝赋值
    VideoReader(const VideoReader&) = delete;
    VideoReader& operator=(const VideoReader&) = delete;
    // 允许移动构造和移动赋值
    VideoReader(VideoReader&&) = default;
    VideoReader& operator=(VideoReader&&) = default;

    /* 函数接口 */
    bool readFrame(cv::Mat &frame);  // 读取一帧
    void Close_Video();              // 关闭视频


private:
    // 使用智能指针管理资源，这里只是声明， ​没有申请内存
    std::unique_ptr<Reader> reader_ptr; 
    // 加载引擎
    void Init_Load_Engine(const int& engine, const string& decodec, const int& accels_2d);
};

#endif // VIDEOREADER_H