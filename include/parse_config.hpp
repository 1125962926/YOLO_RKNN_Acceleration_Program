/*
 * @Author: Li RF
 * @Date: 2025-01-14 19:05:32
 * @LastEditors: Li RF
 * @LastEditTime: 2025-03-16 20:30:38
 * @Description: 
 * Email: 1125962926@qq.com
 * Copyright (c) 2025 Li RF, All Rights Reserved.
 */
#ifndef _PARSE_CONFIG_HPP_
#define _PARSE_CONFIG_HPP_

#include <iostream>
#include <string>
#include <SharedTypes.hpp>

/* 定义配置解析类 */
class ConfigParser {
    
public:
    // 输入格式
    int input_format;  
    // 显示帮助信息
    void print_help(const std::string &program_name) const;
    // 打印配置信息
    void printConfig(const AppConfig &config) const;
    // 解析命令行参数
    AppConfig parse_arguments(int argc, char *argv[]) const;

private:
    // 私有成员（如果有需要可以添加）

};

#endif
