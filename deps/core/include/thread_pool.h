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
#include "lib_export.h"

//  线程池
/*  TODO:
*       1） 增加多种task类型支持
*       2） 增加task执行后返回结果，参考：https://www.cnblogs.com/lzpong/p/6397997.html
*/  
class MYLIB_API ThreadPool {
public:
    ThreadPool(int nums = std::numeric_limits<short>::max());
    ~ThreadPool();

	//  分配任务
	void assign(std::function<void()> task);

    void printSize();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

#endif