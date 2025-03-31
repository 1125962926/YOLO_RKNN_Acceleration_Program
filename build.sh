# 如果命令以非零状态退出，则立即退出，不再执行后续命令
###
 # @Author: Li RF
 # @Date: 2024-11-26 10:07:31
 # @LastEditors: Li RF
 # @LastEditTime: 2025-03-21 12:54:41
 # @Description: 
 # Email: 1125962926@qq.com
 # Copyright (c) 2024 Li RF, All Rights Reserved.
### 
set -e

PROJECT_NAME=yolo_cpp_multi
BUILD_TYPE=Release
GCC_COMPILER=aarch64-linux-gnu
# -g 生成调试信息，-pthread 支持多线程，-Wall 显示所有警告
CMAKE_CXX_FLAGS="-g -Wall"
#VERBOSE=ON

export CC=${GCC_COMPILER}-gcc
export CXX=${GCC_COMPILER}-g++

# $( dirname $0 ) 获取当前脚本所在的目录
# $SOURCE 这里为空，所以获取的是当前目录
# cd -P 命令用于改变当前工作目录到该目录，并解析符号链接
ROOT_PWD=$( cd "$( dirname $0 )" && cd -P "$( dirname "$SOURCE" )" && pwd )

# build
BUILD_DIR=${ROOT_PWD}/build
if [ ! -d "${BUILD_DIR}" ]; then
  mkdir -p ${BUILD_DIR}
fi

# install
# 开发阶段暂时不安装
# INSTALL_DIR=${ROOT_PWD}/install
# if [ ! -d "${INSTALL_DIR}" ]; then
#   mkdir -p ${INSTALL_DIR}
# fi

cd ${BUILD_DIR}
cmake .. \
    -DCMAKE_USER_APP_NAME=${PROJECT_NAME} \
    -DCMAKE_USER_INSTALL_PREFIX=${INSTALL_DIR} \
    -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_C_COMPILER=${CC} \
    -DCMAKE_CXX_COMPILER=${CXX} \
    -DCMAKE_USER_INCLUDE_PATH=${ROOT_PWD}/include \
    -DCMAKE_USER_LIBRARY_PATH=${ROOT_PWD}/lib \
    -DCMAKE_VERBOSE_MAKEFILE=${VERBOSE}
make -j $(nproc)

# 开发阶段暂时不安装
# make install

