/*
 * @Author: Li RF
 * @Date: 2025-01-14 19:05:32
 * @LastEditors: Li RF
 * @LastEditTime: 2025-03-22 16:54:35
 * @Description: 
 * Email: 1125962926@qq.com
 * Copyright (c) 2025 Li RF, All Rights Reserved.
 */
#include <iostream>
#include <cstdlib>
#include <getopt.h>
#include <fstream>

#include "parse_config.hpp"

/**
 * @Description: 检查输入源是否存在
 * @param {string&} name: 
 * @return {*}
 */
static bool isFileExists(string& name) {
    ifstream f(name.c_str());
    return f.good();
}


/**
 * @Description: 显示帮助信息
 * @param {char} *program_name: 程序名称
 * @return {*}
 */
void ConfigParser::print_help(const string &program_name) const { // 常量成员函数承诺不会修改调用它的对象的任何成员变量（函数内只有只读操作）
    cout << "Usage: " << program_name << " [options]" << endl;
    cout << "Options:" << endl;
    cout << "  -m, --model_path <string, require> || Set rknn model path. need to be set" << endl;
    cout << "  -i, --input <int or string, require> || Set input source. int: Camera index, like 0; String: video path. need to be set" << endl;
    cout << "  -a, --accels_2d <int> || Configure the 2D acceleration mode. 1:opencv, 2:RGA. default: 2" << endl;
    cout << "  -t, --threads <string> || Set threads number. default: 1" << endl;
    cout << "  -c, --opencl <bool or int> || Configure the opencl mode. true(1):use opencl, fals(0):use cpu. default: True(1)" << endl;
    cout << "  -d, --decodec <string> || Set decoder. default: h264_rkmpp (option: h264)" << endl;
    cout << "  -r, --read_engine <int or string> || Set input sources read engine. default: 1:ffmpeg (option: 2:opencv)" << endl;
    cout << "  -s, --screen_fps || Show fps on screen" << endl;
    cout << "  -p, --print_fps || Print fps on console" << endl;
    cout << "  -v, --verbose || Enable verbose output" << endl;
    cout << "  -h, --help || Show this help message" << endl;
}

/**
 * @Description: 打印配置信息
 * @param {AppConfig} config: 配置信息
 * @return {*}
 */
// 打印配置信息
void ConfigParser::printConfig(const AppConfig &config) const {
    cout << "​*************************" << endl;
    cout << "Parse Information:" << endl;
    cout << "    Model path: " << config.model_path << endl;
    cout << "    Input source: " << config.input << endl;
    cout << "    Threads: " << config.threads << endl;
    cout << "    Opencl: " << boolalpha << config.opencl << endl; // boolalpha: 将 bool 类型以 true/false 形式输出
    cout << "    Decodec: " << config.decodec << endl;
    cout << "    Screen fps: " << boolalpha << config.screen_fps << endl;
    cout << "    Console fps: " << boolalpha << config.print_fps << endl;

    if (config.accels_2d == ACCELS_2D::ACC_OPENCV)
        cout << "    Accels_2d: opencv"<< endl;
    else if (config.accels_2d == ACCELS_2D::ACC_RGA)
        cout << "    Accels_2d: RGA" << endl;

    if (config.read_engine == READ_ENGINE::EN_FFMPEG)
        cout << "    Read engine: ffmpeg" << endl;
    else if (config.read_engine == READ_ENGINE::EN_OPENCV)
        cout << "    Read engine: opencv" << endl;

    
}

/**
 * @Description: 解析命令行参数
 * @param {int} argc: 命令行参数个数
 * @param {char} *argv: 命令行参数数组
 * @return {*}
 */
AppConfig ConfigParser::parse_arguments(int argc, char *argv[]) const {
    if (argc < 2) {
        this->print_help(argv[0]);
        exit(EXIT_FAILURE);
    }
    AppConfig config;


    /* 定义长选项 */ 
    struct option long_options[] = {
        {"model_path", required_argument, nullptr, 'm'},
        {"input",      required_argument, nullptr, 'i'},
        {"accels_2d",  optional_argument, nullptr, 'a'},
        {"threads",    optional_argument, nullptr, 't'},
        {"opencl",     optional_argument, nullptr, 'c'},
        {"decodec",    optional_argument, nullptr, 'd'},
        {"read_engine",optional_argument, nullptr, 'r'},
        {"screen_fps",   no_argument,       nullptr, 's'},
        {"print_fps",  no_argument,       nullptr, 'p'},
        {"verbose",    no_argument,       nullptr, 'v'},
        {"help",       no_argument,       nullptr, 'h'},
        {nullptr,      0,                 nullptr, 0}
    };

    /* 解析参数 */ 
    int opt;
    // 支持短选项和长选项
    // : 表示该选项需要一个参数，v 和 h 不需要
    // 如果解析到长选项，返回 val 字段的值（即第四列）
    while ((opt = getopt_long(argc, argv, "m:i:a:t:c:d:r:spvh", long_options, nullptr)) != -1) {
        string temp_optarg = "";
        // 拷贝防止被修改
        if (optarg)
            temp_optarg = optarg;

        switch (opt) {
            case 'm': {
                if (!optarg) {
                    cerr << "Error: Missing argument for option: " << static_cast<char>(opt) << endl;
                    exit(EXIT_FAILURE);
                }
                // 检查文件是否存在
                if (!isFileExists(temp_optarg)) {
                    cerr << "Error: File not found: " << temp_optarg << endl;
                    exit(EXIT_FAILURE);
                }
                config.model_path = temp_optarg;
                break;
            }
            case 'i': {
                if (!optarg) {
                    cerr << "Error: Missing argument for option: " << static_cast<char>(opt) << endl;
                    exit(EXIT_FAILURE);
                }
                try {
                    if (temp_optarg.size() == 1) { // 摄像头标号
                        if (isdigit(temp_optarg[0])) 
                            config.input_format = INPUT_FORMAT::IN_CAMERA;
                        else
                            throw invalid_argument("Invalid camera index.");
                    } else{ // 视频路径
                        config.input_format = INPUT_FORMAT::IN_VIDEO;
                    }
                } catch (const exception &e) {
                    exit(EXIT_FAILURE);
                }
                // 检查文件是否存在
                if (!isFileExists(temp_optarg)) {
                    cerr << "Error: File not found: " << temp_optarg << endl;
                    exit(EXIT_FAILURE);
                }
                config.input = temp_optarg;
                break;
            }
            case 'a': {
                if (!optarg) {
                    cerr << "Error: Missing argument for option: " << static_cast<char>(opt) << endl;
                    exit(EXIT_FAILURE);
                }
                try {
                    config.accels_2d = stoi(temp_optarg);
                    if (config.accels_2d != ACCELS_2D::ACC_OPENCV && config.accels_2d != ACCELS_2D::ACC_RGA) 
                        throw invalid_argument("Unsupported hwaccel type.");
                } catch (const exception &e) {
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case 't': {
                if (!optarg) {
                    cerr << "Error: Missing argument for option: " << static_cast<char>(opt) << endl;
                    exit(EXIT_FAILURE);
                }
                config.threads = stoi(temp_optarg);
                break;
            }
            case 'c': {
                if (temp_optarg == "true" || temp_optarg == "1")
                    config.opencl = true;
                else if (temp_optarg == "false" || temp_optarg == "0") 
                    config.opencl = false;
                else {
                    cerr << "Error: Invalid argument for option: " << static_cast<char>(opt) << endl;
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case 'd': {
                if (!optarg) {
                    cerr << "Error: Missing argument for option: " << static_cast<char>(opt) << endl;
                    exit(EXIT_FAILURE);
                }
                config.decodec = temp_optarg;
                break;
            }
            case 'r': {
                if (temp_optarg == "ffmpeg" || temp_optarg == "1")
                    config.read_engine = READ_ENGINE::EN_FFMPEG;
                else if (temp_optarg == "opencv" || temp_optarg == "2")
                    config.read_engine = READ_ENGINE::EN_OPENCV;
                else {
                    cerr << "Error: Unsupported read engine." << endl;
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case 's':
                config.screen_fps = true;
                break;
            case 'p':
                config.print_fps = true;
                break;
            case 'v':
                config.verbose = true;
                break;
            case 'h':
                this->print_help(argv[0]);
                exit(EXIT_SUCCESS);
            default:
                this->print_help(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    if (config.verbose)
        this->printConfig(config);

    return config;
}

