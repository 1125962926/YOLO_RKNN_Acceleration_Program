<!--
 * @Author: Li RF
 * @Date: 2025-03-17 11:01:36
 * @LastEditors: Li RF
 * @LastEditTime: 2025-03-27 16:23:29
 * @Description: 
 * Email: 1125962926@qq.com
 * Copyright (c) 2025 Li RF, All Rights Reserved.
-->
# Reader 架构概述

### 1、​Reader（基类）​
定义了视频读取操作的通用接口（如 open、close、readFrame 等）。
作为所有具体读取器（如 FFmpegReader、OpencvReader 等）的基类，利用多态性实现运行时动态选择具体的实现类。

### 2、​FFmpegReader（Reader 的子类）​
继承自 Reader 基类。
实现了基类中定义的虚函数，具体使用 FFmpeg 库提供的函数来处理视频操作。
在初始化时，可能配置和加载与 FFmpeg 相关的资源或参数。

### ​3、VideoReader（中间件）​
提供给 main 函数或其他上层模块使用的接口。
负责根据配置或输入动态选择并实例化合适的 Reader 子类（如 FFmpegReader 或 OpencvReader）。
封装了对具体 Reader 实例的管理，简化了上层模块对视频读取操作的调用。

### 4、​main 函数
使用 VideoReader 提供的统一接口来操作视频，无需关心底层使用了哪种具体的读取器实现。