/*
 * @Author: Li RF
 * @Date: 2025-03-14 09:16:36
 * @LastEditors: Li RF
 * @LastEditTime: 2025-03-22 16:54:33
 * @Description: 
 * Email: 1125962926@qq.com
 * Copyright (c) 2025 Li RF, All Rights Reserved.
 */
#ifndef SHAREDTYPES_H
#define SHAREDTYPES_H

#include <string>
using namespace std;

/* NPU 数量 */
const int NPU_CORE_NUM = 3;

enum ACCELS_2D {
    ACC_OPENCV = 1,
    ACC_RGA = 2,
};

enum INPUT_FORMAT {
    IN_VIDEO = 1,
    IN_CAMERA = 2,
};

enum READ_ENGINE {
    EN_FFMPEG = 1,
    EN_OPENCV = 2,
};

/* 定义命令行参数结构体 */ 
struct AppConfig {
    // 在屏幕显示 FPS
    bool screen_fps = false;
    // 在终端打印 FPS
    bool print_fps = false;
    // 是否使用opencl
    bool opencl = true;
    // 是否打印命令行参数
    bool verbose = false;
    // 视频加载引擎，默认为 ffmpeg
    int read_engine = READ_ENGINE::EN_FFMPEG;
    // 输入格式，默认为视频
    int input_format = INPUT_FORMAT::IN_VIDEO;
    // 硬件加速，默认为 RGA
    int accels_2d = ACCELS_2D::ACC_RGA;
    // 线程数，默认为1
    int threads = 1;
    // rknn 模型路径
    string model_path = "";
    // 输入源    
    string input = "";
    // 解码器，默认为 h264_rkmpp
    string decodec = "h264_rkmpp";
};


#endif // SHAREDTYPES_H