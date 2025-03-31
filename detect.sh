#!/bin/bash
###
 # @Author: Li RF
 # @Date: 2025-03-09 12:19:51
 # @LastEditors: Li RF
 # @LastEditTime: 2025-03-21 19:08:54
 # @Description: 
 # Email: 1125962926@qq.com
 # Copyright (c) 2025 Li RF, All Rights Reserved.
### 
set -e

model_path=./model/RK3588/yolov5s-640-640.rknn

video_path=../../720p60hz.mp4
#video_path=../../islandBenchmark.mp4

threads=15

read_engine=ffmpeg
#read_engine=opencv

# 2:RGA
#accels=1
accels=2

#opencl=1
opencl=0

runtime_lib_path=${PWD}/lib:${PWD}/lib/ffmpeg

export LD_LIBRARY_PATH=$runtime_lib_path:$LD_LIBRARY_PATH

#gdb --args ./build/yolo_cpp_multi -m $model_path -i $video_path -t $threads -r $read_engine -s -v -a $accels -c $opencl

./build/yolo_cpp_multi -m $model_path -i $video_path -t $threads -r $read_engine -s -v -a $accels -c $opencl
