# 🚀 YOLO RKNN Acceleration Program 
**基于 RK3588 的 YOLO 多线程推理多级硬件加速引擎框架设计** 

*YOLO multi-threaded and hardware-accelerated inference framework based on RKNN* 


## 🏆 性能总结 / Performance Summary
- ### **141 FPS → 151 FPS** 超越基线  
  在原项目最高帧数141帧（C++）的基础上，使用 **RKmpp** 硬件解码和 **RGA** 硬件图像前处理，将推理帧数提高至 **151** 帧。
- ### 关键优化技术 / *Key optimizations*
  - **RKmpp** 硬件解码 / *RKmpp hardware decoding*
  - **RGA** 硬件图像预处理 / *RGA hardware image preprocessing*



## 🛠 技术增强 / Technical Enhancements
- ### 🎭 多态视频加载器（OpenCV/FFmpeg动态切换）  
  *Polymorphic video loader (OpenCV/FFmpeg dynamic switching)*
- ### 🖥️ 命令行参数控制 / *Command-line parameter control*
- ### 🧠 优化内存管理 / *Optimized memory management*



## ✨ 新特性支持 / New features
- ### 图像传输 DMA 优化（计划 2.0 版本加入）
    ```
    状态：✅ 已验证理论并实测成功，待合并进本项目
    ```
    ### 核心改进：
    - **技术路径**：将图像数据的传输方式从 CPU 搬运优化为 DMA（直接内存访问）传输。

    - **核心机制**：通过传递文件描述符（fd）而非拷贝数据本身，实现内存零拷贝，显著提升数据传输效率。

    ### 性能收益：
    - ⚡ **大幅提速**：减少了 CPU 在数据搬运上的参与，直接加快了图像数据的传输速度。
    - 💾 **显著降低 CPU 占用**：将 CPU 从繁重的数据拷贝任务中解放出来，为系统其他任务释放了宝贵的计算资源。

    ### 指导文章：
    - 优化方案的设计与代码实现已完成，相关原理和实现细节已在技术文章中深入探讨：[![CSDN](https://img.shields.io/badge/CSDN-Analysis-blue)](https://blog.csdn.net/plmm__/article/details/152009834?spm=1001.2014.3001.5502)
    
    

## 📚 项目解析 / Project Analysis
- ### CSDN 博客 / *CSDN Articles*
    - **Overview**: [![CSDN](https://img.shields.io/badge/CSDN-Overview-blue)](https://blog.csdn.net/plmm__/article/details/146542002)
    - **Technical Analysis**: [![CSDN](https://img.shields.io/badge/CSDN-Analysis-blue)](https://blog.csdn.net/plmm__/article/details/146556955)



## 📋 快速开始指南 / Quick Start Guide
### 1️⃣ 环境准备 / Prerequisites 
- 开发板需要预装 OpenCV，一般出厂系统都有

    *OpenCV pre-installed (usually included in factory systems)*

### 2️⃣ 测试视频 / Test Video
- 下载 [Baseline](https://github.com/leafqycc/rknn-cpp-Multithreading) Releases 中的测试视频，放项目的根目录

    *Download test video from Baseline Releases and place in project root*

### 3️⃣ (可选) 定频 / (Optional) Frequency Locking
- 可切换至 root 用户运行 `performance.sh` 定频提高性能和稳定性  
    *Run as root: `./performance.sh`*

### 4️⃣ 板端编译 / Board-side Compilation
- 运行 `build.sh`，该脚本会配置并编译 `CMakeLists.txt` 

    *Run `build.sh` to configure and compile `CMakeLists.txt`*

- 没有使用 `install` 进行安装，而是直接执行编译后的程序，节约空间 

    *No `install` used, directly execute the compiled program to save space*

### 5️⃣ 执行推理 / Run Inference
```bash
./detect.sh
```
- 使用 `detect.sh` 进行推理，脚本会根据项目预定的命令行参数进行填写，然后执行编译后的可执行文件

    *Run `detect.sh` to perform inference, the script will fill in the command-line parameters as per the project's predefined parameters, and then execute the compiled executable.*

- 可以根据自己的实际情况修改脚本参数，例如模型路径和视频路径

    *You can modify the script parameters according to your actual situation, such as the model path and video path.*
    

- 也可以直接执行可执行程序，会打印命令行参数提示

    *You can also directly execute the executable program, which will print the command-line parameter prompts.*



## 📂 项目结构 / Project Structure
- `reference` 目录是官方的 demo
- `clean.sh` 用于清除编译生成的文件
- ffmpeg 已经移植到项目中
- `librga` 和 `librknnrt` 已更新至目前的最新版本
- `performance.sh` 是官方的定频脚本

```bash
├── 📜 build.sh
├── 📜 clean.sh
├── 📜 CMakeLists.txt
├── 📜 detect.sh
├── 📂 include/
│   ├── 🖼️ drm_func.h
│   ├── 📹 ffmpeg/
│   ├── ⚙️ parse_config.hpp
│   ├── 🔍 postprocess.h
│   ├── ✨ preprocess.h
│   ├── 📖 reader/
│   ├── 🖥️ rga/
│   ├── 🧠 rknn/
│   ├── 🏊 rknnPool.hpp
│   ├── 🎯 rkYolo.hpp
│   ├── 🔗 SharedTypes.hpp
│   └── 🧵 ThreadPool.hpp
├── 📂 lib/
│   ├── 📹 ffmpeg/
│   ├── 🖥️ librga.so
│   ├── 🔗 librknn_api.so -> librknnrt.so
│   └── 🧠 librknnrt.so
├── 📂 model/
│   ├── 🏷️ coco_80_labels_list.txt
│   └── 🖥️ RK3588/
├── 📜 performance.sh
├── 📂 reference/
│   ├── 📹 ffmpeg_mpp_test.cpp
│   ├── 🖥️ ffmpeg_rga_test.cpp
│   ├── 🎥 main_video.cc
│   └── 🖼️ rga_*.cpp
└── 📂 src/
    ├── 🎯 main.cpp
    ├── ⚙️ parse_config.cpp
    ├── 🔍 postprocess.cpp
    ├── ✨ preprocess.cpp
    ├── 📖 reader/
    └── 🎯 rkYolo.cpp
```

## 📌 基线 / Baseline 
Forked from [leafqycc/rknn-cpp-Multithreading](https://github.com/leafqycc/rknn-cpp-Multithreading)

## 💬 联系方式 / Contact
### ✉️ 开发者 / Developer​​:
Email / QQ: 1125962926@qq.com

欢迎合作优化RKNN加速方案！

*Let's collaborate on optimizing RKNN acceleration!*