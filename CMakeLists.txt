cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 可执行文件名
project(${CMAKE_USER_APP_NAME})
# 设置编译器标志
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# skip 3rd-party lib dependencies
# -Wl 是 GCC 编译器的选项，用于将后面的参数传递给链接器（ld）
# --allow-shlib-undefined 是链接器的选项，表示允许共享库中存在未定义的符号
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-shlib-undefined")

# install target and libraries
set(CMAKE_INSTALL_PREFIX ${CMAKE_USER_INSTALL_PREFIX})
set(CMAKE_SKIP_INSTALL_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

# 用于指定程序运行时查找动态链接库的路径
# RPATH 是在编译时通过链接器选项（-Wl,-rpath）设置的，是直接写入可执行文件或共享库的元数据中的，因此不依赖环境变量
set(CMAKE_INSTALL_RPATH ${CMAKE_USER_LIBRARY_PATH};${CMAKE_USER_LIBRARY_PATH}/ffmpeg)

# lib and include path
# 每次调用都会累计之前的路径，即不会覆盖
link_directories(${CMAKE_USER_LIBRARY_PATH})
include_directories(${CMAKE_USER_INCLUDE_PATH})

# rknn api
set(RKNN_RT_LIB librknnrt.so)
include_directories(${CMAKE_USER_INCLUDE_PATH}/rknn)

# rga
set(RGA_LIB librga.so)
include_directories(${CMAKE_USER_INCLUDE_PATH}/rga)

# opencv
# REQUIRED 关键字意味着如果 CMake 无法找到 OpenCV，构建过程将停止并报告错误
find_package(OpenCV REQUIRED)
message(STATUS "**** OpenCV find: ${OpenCV_VERSION} ****")

# FFmpeg
set(FFMPEG_LIBS avcodec avformat avutil swscale)
link_directories(${CMAKE_USER_LIBRARY_PATH}/ffmpeg)
include_directories(${CMAKE_USER_INCLUDE_PATH}/ffmpeg)

# video reader
include_directories(${CMAKE_USER_INCLUDE_PATH}/reader)

# 匹配 src 目录下的所有 .cpp 文件
file(GLOB SRC_FILES "src/*.cpp")
# 匹配 src/reader 目录下的所有 .cpp 文件
file(GLOB READER_FILES "src/reader/*.cpp")
# 将列表合并
set(ALL_SRC_FILES ${SRC_FILES} ${READER_FILES})
# 编译可执行文件
add_executable(${CMAKE_USER_APP_NAME} ${ALL_SRC_FILES})

# 链接库
target_link_libraries(${CMAKE_USER_APP_NAME}
  ${RKNN_RT_LIB}
  ${RGA_LIB}
  ${OpenCV_LIBS}
  ${FFMPEG_LIBS}
)

# install target and libraries
# install(TARGETS yolov5_muti DESTINATION ./)
# install(PROGRAMS ${RKNN_RT_LIB} DESTINATION lib)
# install(PROGRAMS ${RGA_LIB} DESTINATION lib)
# install(PROGRAMS ${FFMPEG_LIBS} DESTINATION lib)
# install(DIRECTORY model DESTINATION ./)
