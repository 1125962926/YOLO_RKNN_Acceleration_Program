/*
 * @Author: Li RF
 * @Date: 2024-11-26 10:07:30
 * @LastEditors: Li RF
 * @LastEditTime: 2025-03-28 09:16:40
 * @Description: 
 * Email: 1125962926@qq.com
 * Copyright (c) 2024 Li RF, All Rights Reserved.
 */
#include <iostream>
#include <chrono>
#include <filesystem>
#include <signal.h>

#include "opencv2/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>

#include "rkYolo.hpp"
#include "rknnPool.hpp"
#include "parse_config.hpp"
#include "VideoReader.hpp"
#include "SharedTypes.hpp"

// 定期计算 FPS 的间隔时间（毫秒）
#define FPS_INTERVAL 1000

/**
 * @Description: 计算 FPS
 * @return {*}
 */
bool calculateFPS(int& fps) {
    static auto lastReportTime = std::chrono::steady_clock::now();
    static int frameCount = 0;
    
    auto currentTime = std::chrono::steady_clock::now();
    frameCount++;
    
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastReportTime).count();
    
    if (elapsedTime >= FPS_INTERVAL) {
        fps = static_cast<int>(frameCount * 1000.0 / elapsedTime); // 更精确的FPS计算
        frameCount = 0;
        lastReportTime = currentTime;
        return true;
    }
    return false;
}


int main(int argc, char **argv) {

    /* 解析命令行参数 */
    ConfigParser parser;
    AppConfig config = parser.parse_arguments(argc, argv);

    /* 检查 opencl */ 
    if (config.opencl) {
        // 启用 OpenCL
        cv::ocl::setUseOpenCL(true);
        // 检查 OpenCL 是否可用
        if (!cv::ocl::haveOpenCL()) {
            std::cerr << "OpenCL is not available, use CPU instead." << std::endl;
            config.opencl = false;
        }
        else
            config.opencl = true;
    }
    else
        cv::ocl::setUseOpenCL(false);

    /* 初始化视频读取引擎 */
    std::unique_ptr<VideoReader> video_reader_ptr;
    try {
        video_reader_ptr = std::make_unique<VideoReader>(config);
    } catch (const std::exception& e) {
        // 处理异常
        std::cerr << "VideoReader 构造函数错误: " << e.what() << std::endl;
        return -EXIT_FAILURE;
    }
    
    /* 初始化 rknn 线程池 */ 
    rknnPool<rkYolo, cv::Mat, cv::Mat> yolo_pool(config);
    if (yolo_pool.init() != 0) {
        std::cerr << "rknnPool init fail!" << std::endl;
        return -1;
    }

    /* 用于计算 FPS 的参数 */
    int fps = 0;

    /* 处理视频帧 */ 
    while (1) {
        cv::Mat img;
        // auto start_total = std::chrono::high_resolution_clock::now(); // 记录循环开始时间
        // auto start_readFrame = std::chrono::high_resolution_clock::now();

        /* 跳过不需要的帧 */
        if (!video_reader_ptr->readFrame(img)) {
            // 如果已经成功开始处理图像，则代表已处理完所有图像或读取错误
            if (fps != 0)
                break;
            else
                continue;
        }

        // auto end_readFrame = std::chrono::high_resolution_clock::now();
        // auto start_put = std::chrono::high_resolution_clock::now();

        // 放入 rknn 线程池
        if (!img.empty())
            if (yolo_pool.put(img) != 0)
                break;

        // auto end_put = std::chrono::high_resolution_clock::now();
        // auto start_get = std::chrono::high_resolution_clock::now();

        // 从 rknn 线程池获取结果
        // 如果已经成功开始处理图像
        // 如果 get 返回错误，则代表已处理完所有图像或读取错误
        if ((fps != 0) && (yolo_pool.get(img) != 0))
            break;

        // 如果取出的图像为空，则跳过
        if (img.empty())
            continue;
        // auto end_get = std::chrono::high_resolution_clock::now();
        
        // 定期计算帧率，并更新 FPS 和显示
        if(calculateFPS(fps)) {
            if (config.print_fps) {
                std::cout << "FPS: " << fps << std::endl;
            }
        }

        // 将 FPS 文本添加到帧上
        // 为了保证能稳定查看，每张图都显示，所以间隔时间内的帧率是同一个值
        if (config.screen_fps) {
            cv::putText(img, "FPS: " + std::to_string(fps), cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);
        }

        cv::imshow("YOLO demo", img);
        // cv::waitKey 是必需的，它不仅用于检测键盘输入，还用于处理窗口事件和刷新图像
        if (cv::waitKey(1) == 'q') // 延时1毫秒,按q键退出
            break;

        // auto end_total = std::chrono::high_resolution_clock::now();

        // 计算各函数执行时间
        // auto duration_readFrame = std::chrono::duration_cast<std::chrono::milliseconds>(end_readFrame - start_readFrame).count();
        // auto duration_put = std::chrono::duration_cast<std::chrono::milliseconds>(end_put - start_put).count();
        // auto duration_get = std::chrono::duration_cast<std::chrono::milliseconds>(end_get - start_get).count();
        // auto duration_total = std::chrono::duration_cast<std::chrono::milliseconds>(end_total - start_total).count();

        // 打印各函数执行时间
        // std::cout << "readFrame: " << duration_readFrame << " ms" << std::endl;
        // std::cout << "put: " << duration_put << " ms" << std::endl;
        // std::cout << "get: " << duration_get << " ms" << std::endl;
        // std::cout << "Total loop: " << duration_total << " ms" << std::endl;
        // std::cout << "-----------------------------" << std::endl;
    }

    // 等待 rknn 线程池处理完所有图像
    cv::Mat img;
    while(!yolo_pool.get(img));

    // 关闭视频文件
    video_reader_ptr->Close_Video();
    return 0;
}
