/*
 * @Author: Li RF
 * @Date: 2024-11-26 10:07:30
 * @LastEditors: Li RF
 * @LastEditTime: 2025-03-27 22:06:42
 * @Description: 
 * Email: 1125962926@qq.com
 * Copyright (c) 2024 Li RF, All Rights Reserved.
 */

#include <stdio.h>
#include "postprocess.h"
#include "im2d.h"
#include "rga.h"
#include "RgaUtils.h"
#include <iostream>

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"


void letterbox_with_opencl(const cv::Mat &image, cv::UMat &padded_image, BOX_RECT &pads, const float scale, const cv::Size &target_size, const cv::Scalar &pad_color) {
    // 将输入图像转换为 UMat
    cv::UMat uImage = image.getUMat(cv::ACCESS_READ);

    // 调整图像大小
    cv::UMat resized_image;
    cv::resize(uImage, resized_image, cv::Size(), scale, scale);

    if (uImage.empty()) {
        std::cerr << "Error: uImage is empty." << std::endl;
        return;
    }
    if (resized_image.empty()) {
        std::cerr << "Error: resized_image is empty." << std::endl;
        return;
    }

    // 计算填充大小
    int pad_width = target_size.width - resized_image.cols;
    int pad_height = target_size.height - resized_image.rows;

    pads.left = pad_width / 2;
    pads.right = pad_width - pads.left;
    pads.top = pad_height / 2;
    pads.bottom = pad_height - pads.top;

    // 在图像周围添加填充
    cv::copyMakeBorder(resized_image, padded_image, pads.top, pads.bottom, pads.left, pads.right, cv::BORDER_CONSTANT, pad_color);
}

/**
 * @Description: OpenCV 图像预处理
 * @return {*}
 */
void letterbox(const cv::Mat &image, cv::Mat &padded_image, BOX_RECT &pads, const float scale, const cv::Size &target_size, bool Use_opencl, const cv::Scalar &pad_color)
{
    // 图像数据检查
    if (image.empty()) {
        std::cerr << "Error: Input image is empty." << std::endl;
        return;
    }
    // 调整图像大小
    cv::Mat resized_image;

    if (Use_opencl)
    {
        // 预处理图像
        cv::UMat U_padded_image;
        letterbox_with_opencl(image, U_padded_image, pads, scale, target_size, pad_color);

        // 将处理后的图像从 GPU 内存复制回 CPU 内存（如果需要显示）
        // padded_image = U_padded_image.getMat(cv::ACCESS_READ);
        // padded_image = std::move(U_padded_image.getMat(cv::ACCESS_READ));
        padded_image = U_padded_image.getMat(cv::ACCESS_READ).clone(); // 深拷贝
        return ;
    }

    cv::resize(image, resized_image, cv::Size(), scale, scale);

    // 计算填充大小
    int pad_width = target_size.width - resized_image.cols;
    int pad_height = target_size.height - resized_image.rows;

    pads.left = pad_width / 2;
    pads.right = pad_width - pads.left;
    pads.top = pad_height / 2;
    pads.bottom = pad_height - pads.top;

    // 在图像周围添加填充
    cv::copyMakeBorder(resized_image, padded_image, pads.top, pads.bottom, pads.left, pads.right, cv::BORDER_CONSTANT, pad_color);
}



/************************************** RGA *******************************************/
/****************** resize ******************* */
/**
 * @Description: 直接内存映射图像数据，图像的生命周期由用户管理
 * @param {Mat} &image: 输入的源图像，使用 OpenCV 的 Mat 结构表示
 * @param {Mat} &resized_image: 输出的目标图像，经过缩放处理后的图像
 * @return {*} 返回 0 表示成功
 */
int RGA_resize(const cv::Mat &image, cv::Mat &resized_image)
{
    if (image.type() != CV_8UC3) // 8位无符号三通道彩色图像
    {
        printf("source image type is %d!\n", image.type());
        return -1;
    }
    rga_buffer_t src_img;
    rga_buffer_t dst_img;
    size_t img_width = image.cols;
    size_t img_height = image.rows;
    size_t target_width = resized_image.cols;
    size_t target_height = resized_image.rows;
    memset(&src_img, 0, sizeof(src_img));
    memset(&dst_img, 0, sizeof(dst_img));

    // 将源图像和目标图像的数据填充至 rga_buffer_t 结构体（函数内部就是填充 rga_buffer_t）
    // 该函数用于用户自己管理的图像内存
    // OpenCV 的 MAT 格式为 RGB888，没有 A 通道
    src_img = wrapbuffer_virtualaddr((void *)image.data, img_width, img_height, RK_FORMAT_RGB_888);
    // 创建的 RGA 缓冲区，它直接引用了 resized_image.data 的内存地址，这意味着 dst_img 和 resized_image 共享同一块内存
    dst_img = wrapbuffer_virtualaddr((void *)resized_image.data, target_width, target_height, RK_FORMAT_RGB_888);

    // 在配置完毕RGA任务参数后，可以通过该接口校验当前参数是否合法，并根据当前硬件情况判断硬件是否支持
    // src_img	[required] input imageA
    // dst_img	[required] output image
    // srect	[required] src_img crop region
    // drect	[required] dst_img crop region
    // 建议该接口仅在开发调试阶段使用，避免多次校验导致性能损耗
    IM_STATUS STATUS;
    /*STATUS = imcheck(src_img, dst_img, src_rect, dst_rect);
    if (IM_STATUS_NOERROR != STATUS)
    {
        fprintf(stderr, "rga check error! %s", imStrError(STATUS));
        return -1;
    }*/

    // 调⽤RGA实现快速图像缩放操作，将 rga_buffer_t 格式的结构体src、dst传⼊imresize()
    // dst_img 是 resized_image
    STATUS = imresize(src_img, dst_img);
    if (IM_STATUS_SUCCESS != STATUS) {
        fprintf(stderr, "rga resize error! %s", imStrError(STATUS));
        return -1;
    }
    // printf("resizing .... %s\n", imStrError(STATUS));
    return 0;
}

/**
 * @Description: 将图像导入 RGA 内部统一管理内存，而不是用户自己管理
 * @param {Mat} &image: 
 * @param {Mat} &resized_image: 
 * @param {Size} &target_size: 
 * @return {*}
 */
int RGA_handle_resize(const cv::Mat &image, cv::Mat &resized_image) {
    /* 8位无符号三通道彩色图像 */ 
    if (image.type() != CV_8UC3) 
    {
        printf("source image type is %d!\n", image.type());
        return -1;
    }

    rga_buffer_t src_img, dst_img;
    rga_buffer_handle_t src_handle, dst_handle;
    size_t src_width = image.cols;
    size_t src_height = image.rows;
    size_t dst_width = resized_image.cols;
    size_t dst_height = resized_image.rows;
    int src_buf_size, dst_buf_size;
    int src_format = RK_FORMAT_RGB_888;
    int dst_format = RK_FORMAT_RGB_888;
    
    memset(&src_img, 0, sizeof(src_img));
    memset(&dst_img, 0, sizeof(dst_img));
    src_buf_size = src_width * src_height * get_bpp_from_format(src_format);
    dst_buf_size = dst_width * dst_height * get_bpp_from_format(dst_format);

    /* 将缓冲区对应的物理地址信息映射到RGA驱动内部，并获取缓冲区相应的地址信息 */
    // 直接操作Mat.data内存，减少数据复制开销
    src_handle = importbuffer_virtualaddr(image.data, src_buf_size);
    dst_handle = importbuffer_virtualaddr(resized_image.data, dst_buf_size);
    if (src_handle == 0 || dst_handle == 0) {
        printf("importbuffer failed!\n");
        return -1;
    }

    /* 封装为RGA图像结构 */
    src_img = wrapbuffer_handle(src_handle, src_width, src_height, src_format);
    dst_img = wrapbuffer_handle(dst_handle, dst_width, dst_height, dst_format);

    IM_STATUS STATUS;
    /*STATUS = imcheck(src_img, dst_img, {}, {});
    if (IM_STATUS_NOERROR != STATUS) {
        printf("%d, check error! %s", __LINE__, imStrError(STATUS));
        return -1;
    }*/

    /* 执行缩放操作 */ 
    STATUS = imresize(src_img, dst_img);

    /* 释放内存（正确和错误均执行） */ 
    if (src_handle)
        releasebuffer_handle(src_handle);
    if (dst_handle)
        releasebuffer_handle(dst_handle);

    if (IM_STATUS_SUCCESS != STATUS) {
        fprintf(stderr, "rga resize error! %s", imStrError(STATUS));
        return -1;
    }
    return 0;
}

/****************** cvtcolor ******************* */
/******** bgr to rgb ********* */
/**
 * @Description: 将 BGR 格式转换为 RGB 格式（直接映射）
 * @param {uint8_t} *frame_yuv_data: 原始 YUV 图像数据（这里为了减少对 AVFrame 的依赖，只传入 data 数据）
 * @param {Mat} &bgr_image: 转换后的 RGB 图像
 * @return {*}
 */
int RGA_bgr_to_rgb(const cv::Mat& rgb_origin, cv::Mat &bgr_image) {
    rga_buffer_t src_img, dst_img;
    int src_width, src_height, src_format;
    int dst_width, dst_height, dst_format;
    memset(&src_img, 0, sizeof(src_img));
    memset(&dst_img, 0, sizeof(dst_img));

    /* 输入输出大小 */ 
    src_width = rgb_origin.cols;
    src_height = rgb_origin.rows;
    dst_width = bgr_image.cols;
    dst_height = bgr_image.rows;

    /* 将转换前后的格式 */
    src_format = RK_FORMAT_BGR_888;
    dst_format = RK_FORMAT_RGB_888;

    /* 使用图像数据构建 rga_buffer_t 结构体，两个指针指向同一个数据 */
    src_img = wrapbuffer_virtualaddr((void *)rgb_origin.data, src_width, src_height, src_format);
    dst_img = wrapbuffer_virtualaddr((void *)bgr_image.data, dst_width, dst_height, dst_format);

    /* 图像检查 */
    IM_STATUS STATUS;
    /*STATUS = imcheck(src_img, dst_img, {}, {});
    if (IM_STATUS_NOERROR != STATUS) {
        printf("%d, check error! %s", __LINE__, imStrError(STATUS));
        return -1;
    }*/

    /* 将需要转换的格式与 rga_buffer_t 格式的结构体 src、dst ⼀同传⼊ imcvtcolor() */
    STATUS = imcvtcolor(src_img, dst_img, src_format, dst_format);
    if (IM_STATUS_SUCCESS != STATUS) {
        fprintf(stderr, "rga resize error! %s", imStrError(STATUS));
        return -1;
    }
    return 0;
}

/**
 * @Description: 将 BGR 格式转换为 RGB 格式（导入 RGA 内部）
 * @param {uint8_t} *frame_yuv_data: 原始 YUV 图像数据（这里为了减少对 AVFrame 的依赖，只传入 data 数据）
 * @param {Mat} &bgr_image: 转换后的 RGB 图像
 * @return {*}
 */
int RGA_handle_bgr_to_rgb(const cv::Mat& rgb_origin, cv::Mat &bgr_image) {
    rga_buffer_t src_img, dst_img;
    rga_buffer_handle_t src_handle, dst_handle;
    int src_width, src_height, src_format;
    int dst_width, dst_height, dst_format;
    int src_buf_size, dst_buf_size;
    memset(&src_img, 0, sizeof(src_img));
    memset(&dst_img, 0, sizeof(dst_img));

    /* 输入输出大小一致 */ 
    src_width = rgb_origin.cols;
    src_height = rgb_origin.rows;
    dst_width = bgr_image.cols;
    dst_height = bgr_image.rows;

    /* 将转换前后的格式 */
    src_format = RK_FORMAT_BGR_888;
    dst_format = RK_FORMAT_RGB_888;

    /* 计算图像所需要的 buffer 大小 */
    src_buf_size = src_width * src_height * get_bpp_from_format(src_format);
    dst_buf_size = dst_width * dst_height * get_bpp_from_format(dst_format);

    /* 将缓冲区对应的物理地址信息映射到RGA驱动内部，并获取缓冲区相应的地址信息 */
    src_handle = importbuffer_virtualaddr(rgb_origin.data, src_buf_size);
    dst_handle = importbuffer_virtualaddr(bgr_image.data, dst_buf_size);

    /* 图像检查 */
    IM_STATUS STATUS;
    /*STATUS = imcheck(src_img, dst_img, {}, {});
    if (IM_STATUS_NOERROR != STATUS) {
        printf("%d, check error! %s", __LINE__, imStrError(STATUS));
        return -1;
    }*/

    /* 将需要转换的格式与rga_buffer_t格式的结构体src、dst⼀同传⼊imcvtcolor() */
    STATUS = imcvtcolor(src_img, dst_img, src_format, dst_format);

    /* 释放内存（正确和错误均执行） */
    if (src_handle)
        releasebuffer_handle(src_handle);
    if (dst_handle)
        releasebuffer_handle(dst_handle);

    if (IM_STATUS_SUCCESS != STATUS) {
        fprintf(stderr, "rga resize error! %s", imStrError(STATUS));
        return -1;
    }
    return 0;
}

/******** nv12 to bgr ********* */
/**
 * @Description: 将 YUV420SP(NV12) 格式的图像转换为 BGR 格式
 * @param {uint8_t} *frame_yuv_data: 原始 YUV 图像数据（这里为了减少对 AVFrame 的依赖，只传入 data 数据）
 * @param {Mat} &bgr_image: 转换后的 BGR 图像
 * @return {*}
 */
int RGA_yuv420sp_to_bgr(const uint8_t *frame_yuv_data, const int& width, const int& height, cv::Mat &bgr_image) {
    rga_buffer_t src_img, dst_img;
    int src_width, src_height, src_format;
    int dst_width, dst_height, dst_format;
    memset(&src_img, 0, sizeof(src_img));
    memset(&dst_img, 0, sizeof(dst_img));

    /* 输入输出大小 */ 
    src_width = width;
    src_height = height;
    dst_width = bgr_image.cols;
    dst_height = bgr_image.rows;

    /* 将转换前后的格式 */
    // 选自 rk 官方 demo 中的格式
    src_format = RK_FORMAT_YCbCr_420_SP;
    dst_format = RK_FORMAT_BGR_888;

    /* 使用图像数据构建 rga_buffer_t 结构体，两个指针指向同一个数据 */
    src_img = wrapbuffer_virtualaddr((void *)frame_yuv_data, src_width, src_height, src_format);
    dst_img = wrapbuffer_virtualaddr((void *)bgr_image.data, dst_width, dst_height, dst_format);

    /* 图像检查 */
    IM_STATUS STATUS;
    /*STATUS = imcheck(src_img, dst_img, {}, {});
    if (IM_STATUS_NOERROR != STATUS) {
        printf("%d, check error! %s", __LINE__, imStrError(STATUS));
        return -1;
    }*/

    /* 将需要转换的格式与rga_buffer_t格式的结构体src、dst⼀同传⼊imcvtcolor() */
    STATUS = imcvtcolor(src_img, dst_img, src_format, dst_format);
    if (IM_STATUS_SUCCESS != STATUS) {
        fprintf(stderr, "rga resize error! %s", imStrError(STATUS));
        return -1;
    }
    return 0;
}


/**
 * @Description: 将 YUV420SP(NV12) 格式的图像转换为 BGR 格式
 * @param {uint8_t} *frame_yuv_data: 原始 YUV 图像数据（这里为了减少对 AVFrame 的依赖，只传入 data 数据）
 * @param {Mat} &bgr_image: 转换后的 BGR 图像
 * @return {*}
 */
int RGA_handle_yuv420sp_to_bgr(const uint8_t *frame_yuv_data, const int& width, const int& height, cv::Mat &bgr_image) {
    rga_buffer_t src_img, dst_img;
    rga_buffer_handle_t src_handle, dst_handle;
    int src_width, src_height, src_format;
    int dst_width, dst_height, dst_format;
    int src_buf_size, dst_buf_size;
    memset(&src_img, 0, sizeof(src_img));
    memset(&dst_img, 0, sizeof(dst_img));

    /* 输入输出大小 */ 
    src_width = width;
    src_height = height;
    dst_width = bgr_image.cols;
    dst_height = bgr_image.rows;

    /* 将转换前后的格式 */
    // 选自 rk 官方 demo 中的格式
    src_format = RK_FORMAT_YCbCr_420_SP;
    dst_format = RK_FORMAT_BGR_888;

    /* 计算图像所需要的 buffer 大小 */
    src_buf_size = src_width * src_height * get_bpp_from_format(src_format);
    dst_buf_size = dst_width * dst_height * get_bpp_from_format(dst_format);

    /* 将缓冲区对应的物理地址信息映射到RGA驱动内部，并获取缓冲区相应的地址信息 */
    src_handle = importbuffer_virtualaddr((void *)frame_yuv_data, src_buf_size);
    dst_handle = importbuffer_virtualaddr(bgr_image.data, dst_buf_size);

    /* 封装为RGA图像结构 */
    src_img = wrapbuffer_handle(src_handle, src_width, src_height, src_format);
    dst_img = wrapbuffer_handle(dst_handle, dst_width, dst_height, dst_format);

    /* 图像检查 */
    IM_STATUS STATUS;
    /*STATUS = imcheck(src_img, dst_img, {}, {});
    if (IM_STATUS_NOERROR != STATUS) {
        printf("%d, check error! %s", __LINE__, imStrError(STATUS));
        return -1;
    }*/

    /* 将需要转换的格式与rga_buffer_t格式的结构体src、dst⼀同传⼊imcvtcolor() */
    STATUS = imcvtcolor(src_img, dst_img, src_format, dst_format);

    /* 释放内存（正确和错误均执行） */
    if (src_handle)
        releasebuffer_handle(src_handle);
    if (dst_handle)
        releasebuffer_handle(dst_handle);

    if (IM_STATUS_SUCCESS != STATUS) {
        fprintf(stderr, "rga resize error! %s", imStrError(STATUS));
        return -1;
    }
    return 0;
}

