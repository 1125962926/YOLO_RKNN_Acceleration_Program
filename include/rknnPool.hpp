#ifndef RKNNPOOL_H
#define RKNNPOOL_H

#include "ThreadPool.hpp"
#include <vector>
#include <iostream>
#include <mutex>
#include <queue>
#include <memory>
#include "SharedTypes.hpp"

// rknnModel模型类, inputType模型输入类型, outputType模型输出类型
template <typename rknnModel, typename inputType, typename outputType>
class rknnPool
{
private:
    AppConfig config; // 配置参数
    long long id;
    std::mutex idMtx, queueMtx;
    std::unique_ptr<dpool::ThreadPool> pool;
    std::queue<std::future<outputType>> futs;
    std::vector<std::shared_ptr<rknnModel>> models;

protected:
    int getModelId();

public:
    rknnPool(const AppConfig& config);
    int init();
    // 模型推理
    int put(inputType& inputData);
    // 获取推理结果
    int get(outputType& outputData);
    ~rknnPool();
};

template <typename rknnModel, typename inputType, typename outputType>
rknnPool<rknnModel, inputType, outputType>::rknnPool(const AppConfig& config)
{
    this->config = config;
    this->id = 0;
}

template <typename rknnModel, typename inputType, typename outputType>
int rknnPool<rknnModel, inputType, outputType>::init()
{
    try
    {
        // 创建一个线程池，并将其存储在 this->pool 中
        this->pool = std::make_unique<dpool::ThreadPool>(this->config.threads);
        // 创建多个模型实例，并将它们存储在 models 向量中
        for (int i = 0; i < this->config.threads; i++)
            // 创建 rknnModel 实例，并获取一个指向该对象的智能指针，将其存储在 models 向量中
            // 传递 this->modelPath 作为构造函数的参数
            // models.push_back(std::make_shared<rknnModel>(this->modelPath.c_str()));
            models.push_back(std::make_shared<rknnModel>(this->config));
    }// 如果 try 块中的代码抛出了异常，程序会跳转到 catch 块
    catch (const std::bad_alloc &e)
    {
        std::cerr << "Out of memory: " << e.what() << std::endl;
        return -1;
    }
    // 初始化模型
    for (int i = 0, ret = 0; i < this->config.threads; i++)
    {
        // rknnModel->init
        // i = 0 时，初始化模型，i != 0 时，共享权重参数
        ret = models[i]->init(models[0]->get_pctx(), i != 0);
        if (ret != 0)
            return ret;
    }

    return 0;
}

template <typename rknnModel, typename inputType, typename outputType>
int rknnPool<rknnModel, inputType, outputType>::getModelId()
{
    std::lock_guard<std::mutex> lock(idMtx);
    // 循环取模，保证 modelId 在 0 到 threadNum-1 之间
    int modelId = id % this->config.threads;
    id++;
    return modelId;
}

template <typename rknnModel, typename inputType, typename outputType>
int rknnPool<rknnModel, inputType, outputType>::put(inputType& inputData)
{
    // std::lock_guard 会锁定传入的互斥锁，并在析构函数中(作用域结束)自动解锁
    // 只要 std::lock_guard 对象存在，互斥锁就会保持锁定状态
    std::lock_guard<std::mutex> lock(queueMtx);

    // inputType localCopy = inputData.clone();
    
    // futs 存储 std::future 对象
    // std::future 表示一个异步操作的结果
    // infer 执行推理
    // models[this->getModelId()] 要执行infer的实例
    futs.push(pool->submit(&rknnModel::infer, models[this->getModelId()], inputData));
    return 0;
}

template <typename rknnModel, typename inputType, typename outputType>
int rknnPool<rknnModel, inputType, outputType>::get(outputType& outputData)
{
    std::lock_guard<std::mutex> lock(queueMtx);
    if(futs.empty() == true)
        return 1;
    outputData = futs.front().get();
    futs.pop();
    return 0;
}

template <typename rknnModel, typename inputType, typename outputType>
rknnPool<rknnModel, inputType, outputType>::~rknnPool()
{
    while (!futs.empty())
    {
        outputType temp = futs.front().get();
        futs.pop();
    }
}

#endif
