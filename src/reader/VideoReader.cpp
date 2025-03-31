/*
 * @Author: Li RF
 * @Date: 2025-03-15 14:26:22
 * @LastEditors: Li RF
 * @LastEditTime: 2025-03-21 13:30:29
 * @Description: 
 * Email: 1125962926@qq.com
 * Copyright (c) 2025 Li RF, All Rights Reserved.
 */
#include <iostream>
#include "VideoReader.hpp"
#include "OpencvReader.hpp"
#include "FFmpegReader.hpp"

/**
 * @Description: 构造函数，初始化视频加载引擎
 * @param {AppConfig&} config: 命令行参数
 * @return {*}
 */
VideoReader::VideoReader(const AppConfig& config) {
    int engine = config.read_engine;

    /* 如果输入源为摄像头，只使用 OpenCV，由于帧率限制不需要硬件加速 */ 
    if (config.input_format == INPUT_FORMAT::IN_CAMERA)
        engine = READ_ENGINE::EN_OPENCV;

    /* 加载引擎 */
    try {
        this->Init_Load_Engine(engine, config.decodec, config.accels_2d);
    } catch(const std::exception& e) {
        std::cerr << "加载引擎错误: " << e.what() << std::endl;
        throw e;
    }

    /* 打开视频文件 */
    try {
        reader_ptr->openVideo(config.input);
    } catch(const std::exception& e) {
        std::cerr << "打开视频文件错误: " << e.what() << std::endl;
        throw e;
    }
    
    

    
}

/**
 * @Description: 析构函数：释放资源
 * @return {*}
 */
VideoReader::~VideoReader() {
    this->Close_Video();
}

/**
 * @Description: 
 * @param {int&} engine: 初始化加载引擎
 * @param {string&} decodec: 解码器
 * @param {int&} accels_2d: 2d 硬件加速
 * @return {*}
 */
void VideoReader::Init_Load_Engine(const int& engine, const string& decodec, const int& accels_2d) {
    /* 加载引擎 */
    switch (engine)
    {
    case READ_ENGINE::EN_FFMPEG:
        reader_ptr = std::make_unique<FFmpegReader>(decodec, accels_2d);
        break;
    case READ_ENGINE::EN_OPENCV:
        reader_ptr = std::make_unique<OpencvReader>();
        break;
    default:
        throw std::invalid_argument("未知的 Reader 类型");
    }
}

/**
 * @Description: 读取一帧
 * @param {AVFrame} *frame: 取出的帧
 * @return {*}
 */
bool VideoReader::readFrame(cv::Mat& frame) {
    return reader_ptr->readFrame(frame);
}

/**
 * @Description: 关闭视频文件并释放资源
 * @return {*}
 */
void VideoReader::Close_Video() {
    reader_ptr->closeVideo();
}

