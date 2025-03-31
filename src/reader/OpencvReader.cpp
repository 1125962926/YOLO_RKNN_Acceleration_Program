/*
 * @Author: Li RF
 * @Date: 2025-03-16 18:14:05
 * @LastEditors: Li RF
 * @LastEditTime: 2025-03-22 16:13:28
 * @Description: 
 * Email: 1125962926@qq.com
 * Copyright (c) 2025 Li RF, All Rights Reserved.
 */
#include "OpencvReader.hpp"


OpencvReader::OpencvReader() {

}

OpencvReader::~OpencvReader() {

}


/**
 * @Description: 打开视频文件
 * @param {string} &filePath: 
 * @return {*}
 */
void OpencvReader::openVideo(const std::string& filePath) {
    // 打开视频文件
    videoCapture.open(filePath);

    // 检查视频是否成功打开
    if (!videoCapture.isOpened()) {
        std::cerr << "Error: Could not open video file: " << filePath << std::endl;
        return;
    }

}

/**
 * @Description: 获取一帧
 * @param {Mat&} frame: 
 * @return {*}
 */
bool OpencvReader::readFrame(cv::Mat& frame) {
    // 检查视频是否已打开
    if (!videoCapture.isOpened()) {
        std::cerr << "Error: Video is not opened." << std::endl;
        return false;
    }

    // 读取下一帧
    if (!videoCapture.read(frame)) {
        std::cerr << "Warning: Failed to read frame or reached end of video." << std::endl;
        return false;
    }
    return true;
}

/**
 * @Description: 关闭视频文件
 * @return {*}
 */
void OpencvReader::closeVideo() {
    if (videoCapture.isOpened()) {
        videoCapture.release(); // 释放视频文件
        std::cout << "Video file closed." << std::endl;
    }
}