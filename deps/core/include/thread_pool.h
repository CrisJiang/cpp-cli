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
class MYLIB_API ThreadPool {
public:
    ThreadPool(int nums = std::numeric_limits<short>::max());
    ~ThreadPool();

	//  分配任务
	void assign(std::function<void()> task);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

#endif