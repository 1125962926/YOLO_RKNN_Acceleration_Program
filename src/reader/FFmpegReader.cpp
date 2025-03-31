/*
 * @Author: Li RF
 * @Date: 2025-03-16 18:13:52
 * @LastEditors: Li RF
 * @LastEditTime: 2025-03-27 16:31:11
 * @Description:
 * Email: 1125962926@qq.com
 * Copyright (c) 2025 Li RF, All Rights Reserved.
 */

#include "FFmpegReader.hpp"

/**
 * @Description: 构造 FFmpeg 引擎
 * @return {*}
 */
FFmpegReader::FFmpegReader(const string& decodec, const int& accels_2d){
    // 获取 FFmpeg 版本信息
    const char* version = av_version_info();
    // 打印版本信息
    std::cout << "FFmpeg version: " << version << std::endl;
    
    this->decodec = decodec;
    this->accels_2d = accels_2d;
}

/**
 * @Description: 析构 FFmpeg 引擎
 * @return {*}
 */
FFmpegReader::~FFmpegReader() {
    if (tempFrame) 
        av_frame_free(&tempFrame);
    if (packet) 
        av_packet_free(&packet);
    if (codecContext) 
        avcodec_free_context(&codecContext);
    if (formatContext) 
        avformat_close_input(&formatContext);
        // 内部会调用 avformat_free_context(formatContext);
        
}

/**
 * @Description: 打开视频文件
 * @param {string} &filePath: 
 * @return {*}
 */
void FFmpegReader::openVideo(const std::string& filePath) {

    /* 分配一个 AVFormatContext */
    formatContext = avformat_alloc_context();
    if (!formatContext)
        throw std::runtime_error("Couldn't allocate format context");

    /* 打开视频文件 */
    // 并读取头部信息，此时编解码器尚未开启
    if (avformat_open_input(&formatContext, filePath.c_str(), nullptr, nullptr) != 0)
        throw std::runtime_error("Couldn't open video file");

    /* 读取媒体文件的数据包以获取流信息 */
    if (avformat_find_stream_info(formatContext, nullptr) < 0)
        throw std::runtime_error("Couldn't find stream information");

    /* 查找视频流 AVMEDIA_TYPE_VIDEO */
    // -1, -1，意味着没有额外的选择条件，返回值是流索引
    videoStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (videoStreamIndex < 0)
        throw std::runtime_error("Couldn't find a video stream");

    /* 查找解码器 */
    codec = avcodec_find_decoder_by_name(this->decodec.c_str());
    if (!codec)
        throw std::runtime_error("Decoder not found");
    
    /* 初始化编解码器上下文 */ 
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext)
        throw std::runtime_error("Couldn't allocate decoder context");

    /* 获取视频流，它包含了视频流的元数据和参数 */
    video_stream = formatContext->streams[videoStreamIndex];
    
    /* 复制视频参数到解码器上下文 */ 
    if (avcodec_parameters_to_context(codecContext, video_stream->codecpar) < 0)
        throw std::runtime_error("Couldn't copy decoder context");

    /* 自动选择线程数 */
    codecContext->thread_count = 0;
    
    /* 打开编解码器 */ 
    if (avcodec_open2(codecContext, codec, nullptr) < 0)
        throw std::runtime_error("Couldn't open decoder");
    
    /* 分配 AVPacket 和 AVFrame */ 
    tempFrame = av_frame_alloc();
    packet = av_packet_alloc();
    if (!tempFrame || !packet)
        throw std::runtime_error("Couldn't allocate frame or packet"); 
}

/**
 * @Description: 读取一帧
 * @param {Mat&} frame: 取出的帧
 * @return {*}
 */
bool FFmpegReader::readFrame(cv::Mat& frame) {
    // 读取帧
    /*if (av_read_frame(formatContext, packet) < 0) {
        return false; // 没有更多帧
    }*/
    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index != videoStreamIndex) {
            av_packet_unref(packet);
            continue;
        }
        break;
    }

    // 如果是视频流
    if (packet->stream_index != videoStreamIndex) {
        cerr << "Not a video stream: " << packet->stream_index << " != " << videoStreamIndex << endl;
        av_packet_unref(packet);
        return false; // 不是视频流
    }

    // 发送数据包到解码器
    if (avcodec_send_packet(codecContext, packet) < 0) {
        std::cerr << "Failed to send packet to decoder" << std::endl;
        av_packet_unref(packet);
        return false; // 发送数据包失败
    }

    // 接收解码后的帧
    if (avcodec_receive_frame(codecContext, tempFrame) < 0) {
        std::cerr << "Failed to receive frame from decoder" << std::endl;
        av_packet_unref(packet);
        return false;
    }

    // 成功读取一帧，保存在 tempFrame 中
    // 将帧数据转换为 cv::Mat BGR 格式
    if (this->NV12_to_BGR(frame) != 0) {
        std::cerr << "Failed to convert YUV420SP to BGR" << std::endl;
        av_packet_unref(packet);
        return false;
    }

    // 释放数据包
    av_packet_unref(packet);
    
    return true; // 处理完成
}

/**
 * @Description: 转换格式，NV12 转 BGR
 *               该函数内有三种转换方式：
 *                  1. FFmpeg SwsContext 软件转换  
 *                  2. OpenCV 软件转换，可启用 opencl（目前区别不大）
 *                  3. RGA 硬件加速转换
 * @param {Mat&} frame: 
 * @return {*}
 */
int FFmpegReader::NV12_to_BGR(cv::Mat& bgr_frame) {
    if (tempFrame->format != AV_PIX_FMT_NV12) {
        return -EXIT_FAILURE; // 格式错误
    }

    // 设置输出帧的尺寸和格式，防止地址无法访问
    bgr_frame.create(tempFrame->height, tempFrame->width, CV_8UC3);

#if 0 
    // 方式1：使用 FFmpeg SwsContext 软件转换
    return this->FFmpeg_yuv420sp_to_bgr(bgr_frame);
#endif

    // 创建一个完整的 NV12 数据块（Y + UV 交错）
    cv::Mat nv12_mat(tempFrame->height + tempFrame->height / 2, tempFrame->width, CV_8UC1);
    // 将 AVFrame 内的数据，转换为 OpenCV Mat 格式保存
    this->AV_Frame_To_CVMat(nv12_mat);

    // 硬件加速
    if (this->accels_2d == ACCELS_2D::ACC_OPENCV) {
        // 方式2：使用 OpenCV 软件转换
        cv::cvtColor(nv12_mat, bgr_frame, cv::COLOR_YUV2BGR_NV12);
        return EXIT_SUCCESS;
    } else if (this->accels_2d == ACCELS_2D::ACC_RGA) {
        // 方式3：使用 RGA 硬件加速转换
        return RGA_yuv420sp_to_bgr((uint8_t *)nv12_mat.data, tempFrame->width, tempFrame->height, bgr_frame);
    }
    else
        return -EXIT_FAILURE;
}

/**
 * @Description: FFmpeg SwsContext 软件转换（耗时大约 10 ms）
 * @param {AVFrame} *yuv420Frame: 
 * @return {*}
 */
int FFmpegReader::FFmpeg_yuv420sp_to_bgr(cv::Mat& bgr_frame) {
    int srcW = tempFrame->width;
    int srcH = tempFrame->height;

    // 创建 SwsContext 用于颜色空间转换
    SwsContext *swsCtx = sws_getContext(srcW, srcH, AV_PIX_FMT_NV12,
        srcW, srcH, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);
    if (!swsCtx) {
        std::cerr << "Could not initialize the conversion context" << std::endl;
        return -EXIT_FAILURE;
    }

    // 创建一个临时的 AVFrame 用于存储转换后的 BGR 数据
    AVFrame *bgr24Frame = av_frame_alloc();
    if (!bgr24Frame) {
        std::cerr << "Could not allocate the BGR frame" << std::endl;
        sws_freeContext(swsCtx);
        return -EXIT_FAILURE;
    }

    // 填充临时 AVFrame 的数据指针和行尺寸
    av_image_fill_arrays(bgr24Frame->data, bgr24Frame->linesize, bgr_frame.data, AV_PIX_FMT_BGR24, srcW, srcH, 1);
    // 进行颜色空间转换
    sws_scale(swsCtx, (const uint8_t* const*)tempFrame->data, tempFrame->linesize, 0, srcH, bgr24Frame->data, bgr24Frame->linesize);
    
    // 释放资源
    av_frame_free(&bgr24Frame);
    sws_freeContext(swsCtx);

    return EXIT_SUCCESS;
}

/**
 * @Description: 将 AVFrame 内的数据，转换为 OpenCV Mat 格式保存
 * @param {Mat&} frame: 
 * @return {*}
 */
void FFmpegReader::AV_Frame_To_CVMat(cv::Mat& nv12_mat) {

    // 拷贝 Y 分量（确保连续）
    cv::Mat y_plane(tempFrame->height, tempFrame->width, CV_8UC1, tempFrame->data[0], tempFrame->linesize[0]);
    y_plane.copyTo(nv12_mat(cv::Rect(0, 0, tempFrame->width, tempFrame->height)));

#if 1
    // 无论FFmpeg的linesize[1]是否有填充字节，都能正确处理UV数据，兼容FFmpeg内存对齐
    
    // FFmpeg的 linesize[1] 可能有填充字节，导致直接拷贝UV数据时错位。通过 reshape 可以：
    // 先将双通道UV (CV_8UC2) 展开为单通道交错排列的字节流。
    // 再调整形状为 height/2 × width，确保与Y分量拼接后符合NV12标准布局。

    // 将UV分量视为CV_8UC2（双通道），明确表示U和V是交错的（NV12的标准布局）。通过reshape操作确保数据连续性
    cv::Mat uv_plane(tempFrame->height / 2, tempFrame->width / 2, CV_8UC2, tempFrame->data[1], tempFrame->linesize[1]);
    cv::Mat uv_interleaved;

    // 显式处理UV内存布局，确保符合OpenCV对COLOR_YUV2BGR_NV12的要求
    // 参数 1：保持单通道不变
    uv_plane.reshape(1, tempFrame->height / 2 * tempFrame->width / 2 * 2).copyTo(uv_interleaved); // 转换为交错排列
    uv_interleaved.reshape(1, tempFrame->height / 2).copyTo(nv12_mat(cv::Rect(0, tempFrame->height, tempFrame->width, tempFrame->height / 2)));
#else
    // 性能略高（省去reshape操作）
    
    // 拷贝 UV 分量
    cv::Mat uv_plane(tempFrame->height / 2, tempFrame->width, CV_8UC1, tempFrame->data[1], tempFrame->linesize[1]);
    // 直接拷贝UV数据，依赖输入数据的正确性
    uv_plane.copyTo(nv12_mat(cv::Rect(0, tempFrame->height, tempFrame->width, tempFrame->height / 2)));
#endif
}

/**
 * @Description: 关闭视频文件
 * @return {*}
 */
void FFmpegReader::closeVideo() {
    if (codecContext == nullptr || tempFrame == nullptr) {
        return;
    }
    // 刷新解码器，确保所有帧都被解码
    avcodec_send_packet(codecContext, nullptr);
    // 处理剩余的帧
    while (avcodec_receive_frame(codecContext, tempFrame) == 0) {
        std::cout << "Flushed frame with PTS: " << tempFrame->pts << std::endl;
    }
}

/**
 * @Description: 打印视频信息
 * @return {*}
 */
void FFmpegReader::print_video_info(const string& filePath) {
    std::cout << "==== File Format Information ====" << std::endl;
    av_dump_format(formatContext, 0, filePath.c_str(), 0);
}

/**
 * @Description: 获取视频的分辨率
 * @return {*}
 */
int FFmpegReader::getWidth() const {
    return codecContext->width;
}

/**
 * @Description: 获取视频高度
 * @return {*}
 */
int FFmpegReader::getHeight() const {
    return codecContext->height;
}

/**
 * @Description: 获取时间基
 * @return {*}
 */
AVRational FFmpegReader::getTimeBase() const {
    return formatContext->streams[videoStreamIndex]->time_base;
}

/**
 * @Description: 获取帧率
 * @return {*}
 */
double FFmpegReader::getFrameRate() const {
    AVRational frameRate = video_stream->avg_frame_rate;
    // 将 AVRational 转换为 double
    double fps = av_q2d(frameRate); 
    return fps;
}