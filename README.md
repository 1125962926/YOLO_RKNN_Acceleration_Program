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
    #### 核心改进：
    - **技术路径**：将图像数据的传输方式从 CPU 搬运优化为 DMA（直接内存访问）传输。

    - **核心机制**：通过传递文件描述符（fd）而非拷贝数据本身，实现内存零拷贝，显著提升数据传输效率。

    #### 性能收益：
    - ⚡ **大幅提速**：减少了 CPU 在数据搬运上的参与，直接加快了图像数据的传输速度。
    - 💾 **显著降低 CPU 占用**：将 CPU 从繁重的数据拷贝任务中解放出来，为系统其他任务释放了宝贵的计算资源。

    #### 指导文章：
    - 优化方案的设计与代码实现已完成，相关原理和实现细节已在技术文章中深入探讨：[![CSDN](https://img.shields.io/badge/CSDN-Analysis-blue)](https://blog.csdn.net/plmm__/article/details/152009834?spm=1001.2014.3001.5502)
    
## 🎯 未实现的优化 / Unimplemented Optimization
- ### 优化一：模型输入修改
    - #### 方案一：模型转换阶段修改
        - 在 `ONNX -> RKNN` 模型转换阶段，将模型的输入设置为 `NV12`，在实际推理的预处理阶段可以跳过 `RGB` 图像的转换，直接使用硬件解码的 `DRM Frame` 传入 `YOLO` 模型，执行推理和后处理，叠加推理结果（画框）。
        - 这个方案的调整相对可行，但是模型的输入格式非常依赖 `rknn-toolkit2` 的适配程度，大概率需要手动修改 `rknn-toolkit2` 适配程序。本质上也是有 `RGB` 的转换，不过是内嵌到每次模型的推理中，作为一个固定的模块合并到 `NPU` 中执行。这里还需要处理好数据转换的 `stream`，应该也是一个比较麻烦的部分。

    - #### 方案二：重构 YOLO 推理模型的输入格式
        - 重新训练 `YOLO` 推理模型，将传统的 `RGB` 输入数据修改为 `NV12` 输入数据，重写图像数据解析的部分，完全定制适配 `NV12` 输入图像。
        - 这个方案的代价有点大，但是优点也很明显。缺点是修改模型需要对 `YOLO` 的模型结构有整体的认识，明确知道图像输入数据的修改，以及图像数据的处理。优点是全链路优化，从解码到显示，中间不存在图像数据的转换，完全适配嵌入式设备和需要解码的场景。

- ### 优化二：图像显示提速
    - #### 方案一：DRM/KMS 直接合成显示
        - 使用 libdrm + rockchip_drm.h，将解码 buffer 作为 overlay plane 直接送显。
        - 可叠加推理结果（画框）：先用 GPU（Mali-G610）渲染 OSD 到另一个 buffer，再用 KMS 合成。
    
    - #### 方案二：EGLImage + OpenGL ES 渲染
        - 将 dma-buf 导入为 EGLImageKHR，绑定到 OpenGL ES 纹理。
        - 在 GPU 上：
            - 渲染原始视频帧
            - 叠加检测框/文字（用 shader 或 Cairo/EGL）
        - 最终通过 GBM + KMS 或 Wayland/Weston 输出。

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