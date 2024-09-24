#ifndef _THREAD_POOL_
#define _THREAD_POOL_

#include <thread>
#include <mutex>
#include <shared_mutex>
#include <unordered_set >
#include <list>
#include <set>
#include <functional>
#include <chrono>
#include <any>
#include <iostream>
#include <stdint.h>
#include "lib_export.h"
#include "core/include/singleton.h"

//  线程池
/*  TODO:
*       1） 增加多种task类型支持
*       2） 增加task执行后返回结果，参考：https://www.cnblogs.com/lzpong/p/6397997.html
*/  
class MYLIB_API ThreadPool : public Singleton<ThreadPool>{
    friend Singleton<ThreadPool>;
protected:
    ThreadPool(int nums = std::thread::hardware_concurrency());
    ~ThreadPool();

public:
	//  分配任务
	void assign(std::function<void()> task);

    //  打印线程池信息
    void printSize();

    //  安全的关闭线程池
    void stop();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

#endif