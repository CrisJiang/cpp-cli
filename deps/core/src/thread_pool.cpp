#include <unordered_set>
#include "include/thread_pool.h"
#include "include/log.h"

class ThreadPool::Impl {
public:
	Impl(ThreadPool* thiz) :
		thiz_(thiz){

	};

	struct stTask {
		std::function<void()> fn_;
	};

	struct stThread {
		std::thread thread_;
		std::atomic<bool> is_stop_{ false };
	};
	//  创建线程池
	void create_pool(int threads);
	//  获取一个空闲线程
	std::shared_ptr<stThread> get_idle_thread();
	//  移动到空闲列表
	void move_to_idle(std::shared_ptr<stThread> thread);
	//  移动到忙碌列表
	void move_to_busy(std::shared_ptr<stThread> thread);

	//  获取空闲线程数量
	int get_idle_thread_numbers();
	//  获取忙碌线程数量
	int get_busy_thread_numbers();
	//  获取一个任务
	std::shared_ptr<stTask> get_task();
	void add_task(std::function<void()>);

	ThreadPool* thiz_;

	int threads_num_{ 0 };

	//  任务队列
	std::list<std::shared_ptr<stTask>> task_list_;

	std::unordered_set<std::shared_ptr<stThread>> idle_threads_;
	std::unordered_set<std::shared_ptr<stThread>> busy_threads_;

	mutable std::mutex  mutex_;

	//  条件变量 - 用于生产者-消费者 从task队列中取出任务
	std::condition_variable idle_cond_;
	std::condition_variable task_cond_;
};

void ThreadPool::Impl::create_pool(int threads)
{
	threads_num_ = threads;
	for (int i = 0; i < threads_num_; i++) {
		auto thread = std::make_shared<stThread>();
		//  创建std::thread
		std::thread threadImpl([this, thread]() {
			while (!thread->is_stop_) {
				//  获取一个task
				auto task = this->get_task();

				//  执行task
				if (task)
					task->fn_();

				//  设置回空闲 - 本线程需要设置回来空闲
				move_to_idle(thread);
			}
		});
		thread->thread_ = std::move(threadImpl);
		thread->thread_.detach();
		//  加入到空闲队列中
		idle_threads_.insert(thread);
	}
}

void ThreadPool::Impl::move_to_idle(std::shared_ptr<stThread> thread)
{
	//LOG_DEBUG("move_to_idle_list");
	auto lock = std::unique_lock(mutex_);
	if (busy_threads_.size() == 0)
		return;
	//  提取原有的元素，加入到idle中
	auto e = busy_threads_.extract(thread);
	if (e) {
		idle_threads_.insert(std::move(e));
		//  通知等待线程
		idle_cond_.notify_one();
	}
}

void ThreadPool::Impl::move_to_busy(std::shared_ptr<stThread> thread)
{
	//LOG_DEBUG("move_to_busy_list");
	auto lock = std::unique_lock(mutex_);
	if (idle_threads_.size() == 0)
		return;
	auto e = idle_threads_.extract(thread);
	if (e)
		busy_threads_.insert(std::move(e));
}

int ThreadPool::Impl::get_idle_thread_numbers()
{
	auto lock = std::unique_lock(mutex_);
	return idle_threads_.size();
}

int ThreadPool::Impl::get_busy_thread_numbers()
{
	auto lock = std::unique_lock(mutex_);
	return busy_threads_.size();
}

std::shared_ptr<ThreadPool::Impl::stTask> ThreadPool::Impl::get_task()
{
	auto lock = std::unique_lock(mutex_);
	//  等待任务就绪
	task_cond_.wait(lock, [this] { return !task_list_.empty(); });

	//  等待任务
	if (task_list_.empty()) {
		return nullptr;
	}

	// 弹出一个任务
	auto task = task_list_.front();
	task_list_.pop_front();

	return task;
}

void ThreadPool::Impl::add_task(std::function<void()> task)
{
	std::unique_lock<std::mutex> lock(mutex_);
	auto task_t = std::make_shared<Impl::stTask>();
	task_t->fn_ = task;
	task_list_.emplace_back(task_t);
	task_cond_.notify_one();
}

std::shared_ptr<ThreadPool::Impl::stThread> ThreadPool::Impl::get_idle_thread()
{
	auto lock = std::unique_lock(mutex_);

	//  等待空闲线程
	idle_cond_.wait(lock, [this] { return !idle_threads_.empty(); });

	//  获取一个空闲线程
	auto it = idle_threads_.begin();
	auto& idle_thread = *it;
	//  从空闲列表中移除
	idle_threads_.erase(it);

	return idle_thread;
}

ThreadPool::ThreadPool(int nums)
	: impl_(new Impl(this)) {
	LOG_DEBUG("create thread nums:{}", nums);
	impl_->create_pool(nums);
}

ThreadPool::~ThreadPool()
{
	//_run = false;
	impl_->idle_cond_.notify_all(); // 唤醒所有线程执行
	for (auto& thread : impl_->idle_threads_) {
		if (thread->thread_.joinable())
			thread->thread_.join(); // 等待任务结束， 前提：线程一定会执行完
	}
}

void ThreadPool::assign(std::function<void()> task)
{
	impl_->add_task(task);
	//  获取空闲线程
	auto thread = impl_->get_idle_thread();
	if (thread) {
		impl_->move_to_busy(thread);
	}
}
