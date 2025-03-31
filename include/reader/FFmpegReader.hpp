/*
 * @Author: Li RF
 * @Date: 2025-03-16 18:12:00
 * @LastEditors: Li RF
 * @LastEditTime: 2025-03-26 21:23:27
 * @Description: 
 * Email: 1125962926@qq.com
 * Copyright (c) 2025 Li RF, All Rights Reserved.
 */
// FFmpegReader.h
#ifndef FFMPEGREADER_H
#define FFMPEGREADER_H

#include <iostream>
#include "Reader.hpp"
#include "preprocess.h"
#include "SharedTypes.hpp"

#include <opencv2/opencv.hpp>
extern "C" {
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

/**
 * @Description: FFmpeg 引擎
 * @return {*}
 */
class FFmpegReader : public Reader {
public:
    FFmpegReader(const string& decodec, const int& accels_2d);
    ~FFmpegReader() override;

    void openVideo(const std::string& filePath) override;
    bool readFrame(cv::Mat& frame) override;
    void closeVideo() override;

    // 获取视频信息
    void print_video_info(const string& filePath);
    int getWidth() const;
    int getHeight() const;
    AVRational getTimeBase() const;
    double getFrameRate() const;
    
private:
    
    string decodec;                             // 解码器
    int accels_2d;                              // 2D 硬件加速类型
    AVFormatContext *formatContext = nullptr;   // 输入文件的上下文
    AVCodecContext *codecContext = nullptr;     // 解码器上下文
    const AVCodec* codec = nullptr;             // 解码器
    int videoStreamIndex = -1;                  // 视频流的索引
    AVStream *video_stream;                     // 视频流
    AVFrame *tempFrame = nullptr;               // 临时帧（用于解码）
    AVPacket *packet = nullptr;                 // 数据包

    int NV12_to_BGR(cv::Mat& bgr_frame);
    int FFmpeg_yuv420sp_to_bgr(cv::Mat& bgr_frame);
    void AV_Frame_To_CVMat(cv::Mat& nv12_mat);
};

#endif // FFMPEGREADER_H