#include <stdio.h>
#include <iostream>
#include <mutex>
#include <memory>
#include <fstream>
#include <vector>
#include "rknn_api.h"

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "postprocess.h"
#include "preprocess.h"
#include "rkYolo.hpp"

/**
 * @Description: 设置模型需要绑定的核心
 * @return {*}
 */
static int get_core_num() {
    static int core_num = 0;
    static std::mutex mtx;

    std::lock_guard<std::mutex> lock(mtx);

    int temp = core_num % NPU_CORE_NUM;
    core_num++;
    return temp;
}

/**
 * @Description: 打印 tensor 的格式
 * @param {rknn_tensor_attr} *attr: 
 * @return {*}
 */
static void dump_tensor_attr(rknn_tensor_attr *attr) {
    std::string shape_str = attr->n_dims < 1 ? "" : std::to_string(attr->dims[0]);
    for (int i = 1; i < attr->n_dims; ++i)
    {
        shape_str += ", " + std::to_string(attr->dims[i]);
    }

    printf("  index=%d, name=%s, n_dims=%d, dims=[%s], n_elems=%d, size=%d, w_stride = %d, size_with_stride=%d, fmt=%s, "
           "type=%s, qnt_type=%s, "
           "zp=%d, scale=%f\n",
           attr->index, attr->name, attr->n_dims, shape_str.c_str(), attr->n_elems, attr->size, attr->w_stride,
           attr->size_with_stride, get_format_string(attr->fmt), get_type_string(attr->type),
           get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}

/*static unsigned char *load_data(FILE *fp, size_t ofst, size_t sz)
{
    unsigned char *data;
    int ret;

    data = NULL;

    if (NULL == fp)
    {
        return NULL;
    }

    ret = fseek(fp, ofst, SEEK_SET);
    if (ret != 0)
    {
        printf("blob seek failure.\n");
        return NULL;
    }

    data = (unsigned char *)malloc(sz);
    if (data == NULL)
    {
        printf("buffer malloc failure.\n");
        return NULL;
    }
    ret = fread(data, 1, sz, fp);
    return data;
}

static unsigned char *load_model(const char *filename, int *model_size)
{
    FILE *fp;
    unsigned char *data;

    fp = fopen(filename, "rb");
    if (NULL == fp)
    {
        printf("Open file %s failed.\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);

    data = load_data(fp, 0, size);

    fclose(fp);

    *model_size = size;
    return data;
}*/

// （）
/**
 * @Description: 获取文件大小
 * @param {string&} filename: 
 * @return {size_t}: 返回字节数，失败返回0
 */
static size_t get_file_size(const std::string& filename) {
    // std::ios::ate：打开文件后立即将文件指针移动到文件末尾（at end）
    std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
    if (!ifs.is_open())
        return 0;
    
    // 通过文件尾定位获取大小
    size_t size = ifs.tellg();
    ifs.close();
    return size;
}

/**
 * @Description: 加载文件数据
 * @param {ifstream&} ifs: 
 * @param {size_t} offset: 
 * @param {unsigned char*} buffer: 
 * @param {size_t} size: 
 * @return {*}
 */
static bool load_data(std::ifstream& ifs, size_t offset, unsigned char* buffer, size_t size) {
    if (!ifs.is_open()) {
        std::cerr << "File stream not open" << std::endl;
        return false;
    }
    // 定位到指定位置
    ifs.seekg(offset, std::ios::beg);
    if (ifs.fail()) {
        std::cerr << "Seek failed at offset " << offset << std::endl;
        return false;
    }

    ifs.read(reinterpret_cast<char*>(buffer), size);
    // ifs.gcount()：返回实际读取的字节数
    if (ifs.gcount() != static_cast<std::streamsize>(size)) {
        std::cerr << "Read failed, expected " << size << " bytes, got " << ifs.gcount() << std::endl;
        return false;
    }
    return true;
}

/**
 * @Description: 加载模型
 * @param {string} &filename: 
 * @param {unsigned char} *buffer: 
 * @param {size_t&} buffer_size: 
 * @return {*}
 */
static bool load_model(const std::string &filename, unsigned char *buffer, const size_t& buffer_size) {
    // std::ios::binary：以二进制模式打开文件
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs)
    {
        std::cerr << "Failed to open: " << filename << std::endl;
        return false;
    }
    if (buffer_size == 0)
    {
        std::cerr << "Failed to open: " << filename << std::endl;
        return false;
    }

    return load_data(ifs, 0, buffer, buffer_size);
}

/**
 * @Description: 构造函数
 * @param {AppConfig&} config: 
 * @return {*}
 */
rkYolo::rkYolo(const AppConfig& config) {
    this->config = config;           // 配置参数
    nms_threshold = NMS_THRESH;      // 默认的NMS阈值
    box_conf_threshold = BOX_THRESH; // 默认的置信度阈值
}

/**
 * @Description: 每个线程都要执行一次，初始化模型
 * @param {rknn_context} *ctx_in: 
 * @param {bool} share_weight: 
 * @return {*}
 */
int rkYolo::init(rknn_context *ctx_in, bool share_weight) {
    // std::cout << "Loading model..." << std::endl;
    
    // 模型参数复用（为 false 时也代表此时为第一个线程）
    if (share_weight == true)
        ret = rknn_dup_context(ctx_in, &ctx);
    else {
        size_t file_size = get_file_size(this->config.model_path);
        if (file_size == 0) {
            std::cerr << "Failed to get file size" << std::endl;
            return -1;
        }
        // 模型数据缓冲区
        unsigned char* model_data = new unsigned char[file_size];
        if (!load_model(this->config.model_path, model_data, file_size)) {
            std::cerr << "Failed to load model" << std::endl;
            delete[] model_data;
            return -1;
        }
        ret = rknn_init(&ctx, model_data, file_size, 0, NULL);
        delete[] model_data;
    }
        
    if (ret < 0) {
        std::cerr << "rknn_init error ret=" << ret << std::endl;
        return -1;
    }

    // 设置模型绑定的核心
    rknn_core_mask core_mask;
    switch (get_core_num()) {
    case 0:
        core_mask = RKNN_NPU_CORE_0;
        break;
    case 1:
        core_mask = RKNN_NPU_CORE_1;
        break;
    case 2:
        core_mask = RKNN_NPU_CORE_2;
        break;
    }
    ret = rknn_set_core_mask(ctx, core_mask);
    if (ret < 0) {
        std::cerr << "rknn_set_core_mask error ret=" << ret << std::endl;
        return -1;
    }

    rknn_sdk_version version;
    ret = rknn_query(ctx, RKNN_QUERY_SDK_VERSION, &version, sizeof(rknn_sdk_version));
    if (ret < 0) {
        std::cerr << "rknn_init error ret=" << ret << std::endl;
        return -1;
    }
    // 只需要第一个线程打印
    if (!share_weight)
        cout << "sdk version: " << version.api_version << " driver version: " << version.drv_version << endl;

    // 获取模型输入输出参数
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret < 0) {
        std::cerr << "rknn_init error ret=" << ret << std::endl;
        return -1;
    }
    // 只需要第一个线程打印
    if (!share_weight)
        cout << "model input num: " << io_num.n_input << ", output num: " << io_num.n_output << endl;

    // 设置输入参数
    // input_attrs = (rknn_tensor_attr *)calloc(io_num.n_input, sizeof(rknn_tensor_attr));
    input_attrs = std::make_unique<rknn_tensor_attr[]>(io_num.n_input);
    for (int i = 0; i < io_num.n_input; i++)
    {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret < 0) {
            cout << "rknn_query input attr failed" << endl;
            return -1;
        }
        // dump_tensor_attr(&(input_attrs[i]));
    }

    // 设置输出参数
    // output_attrs = (rknn_tensor_attr *)calloc(io_num.n_output, sizeof(rknn_tensor_attr));
    output_attrs = std::make_unique<rknn_tensor_attr[]>(io_num.n_output); 
    for (int i = 0; i < io_num.n_output; i++)
    {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        // dump_tensor_attr(&(output_attrs[i]));
    }

    if (input_attrs[0].fmt == RKNN_TENSOR_NCHW) {
        // 只需要第一个线程打印
        if (!share_weight)
            cout << "model input fmt is NCHW" << endl;
        channel = input_attrs[0].dims[1];
        height = input_attrs[0].dims[2];
        width = input_attrs[0].dims[3];
    }
    else {
        // 只需要第一个线程打印
        if (!share_weight)
            cout << "model input fmt is NHWC" << endl;
        height = input_attrs[0].dims[1];
        width = input_attrs[0].dims[2];
        channel = input_attrs[0].dims[3];
    }
    // 只需要第一个线程打印
    if (!share_weight)
        cout << "model input height=" << height << ", width=" << width << ", channel=" << channel << endl;

    memset(inputs, 0, sizeof(inputs));
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].size = width * height * channel;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].pass_through = 0;

    return 0;
}

rknn_context *rkYolo::get_pctx()
{
    return &ctx;
}

cv::Mat rkYolo::infer(cv::Mat orig_img)
{
    std::lock_guard<std::mutex> lock(mtx);
    // 创建需要 resize 的空图像，这里只申明，不申请内存
    cv::Mat resized_img;
    // 创建 rgb 空图像
    cv::Mat rgb_img(orig_img.rows, orig_img.cols, CV_8UC3);

    BOX_RECT pads;
    memset(&pads, 0, sizeof(BOX_RECT));

    // YOLO 推理需要 RGB 格式，后处理需要 BGR 格式
    // 即使前处理时提前转换为 RGB，后处理部分任然需要转换为 BGR，需要在本函数中保留两种格式
    if (this->config.accels_2d == ACCELS_2D::ACC_OPENCV) {
        cv::cvtColor(orig_img, rgb_img, cv::COLOR_BGR2RGB);
    }
    else if (this->config.accels_2d == ACCELS_2D::ACC_RGA) {
        if (RGA_bgr_to_rgb(orig_img, rgb_img) != 0) {
            cout << "RGA_bgr_to_rgb error" << endl;
            return cv::Mat();
        }
    }
    else {
        cout << "Unsupported 2D acceleration" << endl;
        return cv::Mat();
    }
    
    // 计算缩放比例
    float scale_w = (float)width / rgb_img.cols;
    float scale_h = (float)height / rgb_img.rows;

    // 图像缩放
    if (orig_img.cols != width || orig_img.rows != height)
    {
        // 如果需要缩放，再对 resized_img 申请大小，节约内存开销
        resized_img.create(height, width, CV_8UC3);
        if (this->config.accels_2d == ACCELS_2D::ACC_OPENCV)
        {
            // 打包模型输入尺寸
            cv::Size target_size(width, height);
            float min_scale = std::min(scale_w, scale_h);
			scale_w = min_scale;
			scale_h = min_scale;
			letterbox(rgb_img, resized_img, pads, min_scale, target_size, this->config.opencl);
        }
        else if (this->config.accels_2d == ACCELS_2D::ACC_RGA)
        {
            ret = RGA_resize(rgb_img, resized_img);
            if (ret != 0) {
                cout << "resize_rga error" << endl;
            }
        }
        else {
            cout << "Unsupported 2D acceleration" << endl;
            return cv::Mat();
        }
        inputs[0].buf = resized_img.data;
    }
    else
    {
        inputs[0].buf = rgb_img.data;
    }
    
    rknn_inputs_set(ctx, io_num.n_input, inputs);
    
    rknn_output outputs[io_num.n_output];
    memset(outputs, 0, sizeof(outputs));
    for (int i = 0; i < io_num.n_output; i++)
    {
        outputs[i].want_float = 0;
    }

    // 模型推理
    ret = rknn_run(ctx, NULL);
    ret = rknn_outputs_get(ctx, io_num.n_output, outputs, NULL);

    // 后处理
    detect_result_group_t detect_result_group;
    std::vector<float> out_scales;
    std::vector<int32_t> out_zps;
    for (int i = 0; i < io_num.n_output; ++i)
    {
        out_scales.push_back(output_attrs[i].scale);
        out_zps.push_back(output_attrs[i].zp);
    }
    post_process((int8_t *)outputs[0].buf, (int8_t *)outputs[1].buf, (int8_t *)outputs[2].buf, height, width,
                 box_conf_threshold, nms_threshold, pads, scale_w, scale_h, out_zps, out_scales, &detect_result_group);

    // 绘制框体
    char text[256];
    for (int i = 0; i < detect_result_group.count; i++)
    {
        detect_result_t *det_result = &(detect_result_group.results[i]);
        sprintf(text, "%s %.1f%%", det_result->name, det_result->prop * 100);
        // 打印预测物体的信息/Prints information about the predicted object
        // printf("%s @ (%d %d %d %d) %f\n", det_result->name, det_result->box.left, det_result->box.top,
        //        det_result->box.right, det_result->box.bottom, det_result->prop);
        int x1 = det_result->box.left;
        int y1 = det_result->box.top;
        int x2 = det_result->box.right;
        int y2 = det_result->box.bottom;
        // rectangle 和 putText 需要 BGR 格式
        rectangle(orig_img, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(256, 0, 0, 256), 3);
        putText(orig_img, text, cv::Point(x1, y1 + 12), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 255));
    }

    ret = rknn_outputs_release(ctx, io_num.n_output, outputs);
    return orig_img;
}

rkYolo::~rkYolo()
{
    ret = rknn_destroy(ctx);
    if (ret < 0) {
        cout << "rknn_destroy fail! ret=" << ret << endl;
    }

    // 更新为智能指针
    /*if (input_attrs)
    {
        free(input_attrs);
        input_attrs = nullptr;
    }

    if (output_attrs)
    {
        free(output_attrs);
        output_attrs = nullptr;
    }*/
}
