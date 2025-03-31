# YOLO RKNN Acceleration Program
基于 RK3588 的 YOLO 多线程推理多级硬件加速引擎框架设计 

YOLO multi-threaded and hardware-accelerated inference framework based on RKNN 

---
# Baseline
From leafqycc：
[rknn-cpp-Multithreading](https://github.com/leafqycc/rknn-cpp-Multithreading)

# Summarize
- 在作者的最高帧数141帧（C++）的基础上，使用 **RKmpp** 硬件解码和 **RGA** 硬件图像前处理，将推理帧数提高至 **151** 帧，目前还在优化中。

- 使用多态实现 OpenCV 和 FFmpeg 实现视频加载器动态切换；增加了命令行参数解析，将很多可选功能的开关控制权开放到命令行；优化内存管理。

# Project analysis
CSDN：

### Overview:
https://blog.csdn.net/plmm__/article/details/146542002?spm=1001.2014.3001.5501

### Analysis
https://blog.csdn.net/plmm__/article/details/146556955?spm=1001.2014.3001.5501

# Instruction Manual
### （1）预装 OpenCV
开发板需要预装 OpenCV，一般出厂系统都有。

### （2）测试视频
下载 [Baseline](https://github.com/leafqycc/rknn-cpp-Multithreading) Releases 中的测试视频，放项目的根目录。

### （3）定频（可选）
可切换至 root 用户运行 performance.sh 定频提高性能和稳定性，我一般不使用。

### （4）板端编译
运行 `build.sh`，该脚本会配置并编译 `CMakeLists.txt`。

没有使用 `install` 进行安装，而是直接执行编译后的程序，节约空间。

### （5）执行推理
使用 `detect.sh` 进行推理，脚本会根据项目预定的命令行参数进行填写，然后执行编译后的可执行文件。可以根据自己的实际情况修改脚本参数，例如模型路径和视频路径。

也可以直接执行可执行程序，会打印命令行参数提示。

# Directory structure
- `reference` 目录是官方的 demo
- `clean.sh` 用于清除编译生成的文件
- ffmpeg 已经移植到项目中
- `librga` 和 `librknnrt` 已更新至目前的最新版本
- `performance.sh` 是官方的定频脚本

```bash
├── build.sh
├── clean.sh
├── CMakeLists.txt
├── detect.sh
├── include
│   ├── drm_func.h
│   ├── ffmpeg
│   ├── parse_config.hpp
│   ├── postprocess.h
│   ├── preprocess.h
│   ├── reader
│   ├── rga
│   ├── rknn
│   ├── rknnPool.hpp
│   ├── rkYolo.hpp
│   ├── SharedTypes.hpp
│   └── ThreadPool.hpp
├── lib
│   ├── ffmpeg
│   ├── librga.so
│   ├── librknn_api.so -> librknnrt.so
│   └── librknnrt.so
├── model
│   ├── coco_80_labels_list.txt
│   └── RK3588
├── performance.sh
├── reference
│   ├── ffmpeg_mpp_test.cpp
│   ├── ffmpeg_rga_test.cpp
│   ├── main_video.cc
│   ├── rga_cvtcolor_csc_demo.cpp
│   ├── rga_cvtcolor_demo.cpp
│   ├── rgaImDemo.cpp
│   └── rga_resize_demo.cpp
└── src
    ├── main.cpp
    ├── parse_config.cpp
    ├── postprocess.cpp
    ├── preprocess.cpp
    ├── reader
    └── rkYolo.cpp
```

# Contact me
QQ and e-mail：1125962926@qq.com
