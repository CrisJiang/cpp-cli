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
	//  �����̳߳�
	void create_pool(int threads);
	//  ��ȡһ�������߳�
	std::shared_ptr<stThread> get_idle_thread();
	//  �ƶ��������б�
	void move_to_idle(std::shared_ptr<stThread> thread);
	//  �ƶ���æµ�б�
	void move_to_busy(std::shared_ptr<stThread> thread);

	//  ��ȡ�����߳�����
	int get_idle_thread_numbers();
	//  ��ȡæµ�߳�����
	int get_busy_thread_numbers();
	//  ��ȡһ������
	std::shared_ptr<stTask> get_task();
	void add_task(std::function<void()>);

	ThreadPool* thiz_;

	int threads_num_{ 0 };

	//  �������
	std::list<std::shared_ptr<stTask>> task_list_;

	std::unordered_set<std::shared_ptr<stThread>> idle_threads_;
	std::unordered_set<std::shared_ptr<stThread>> busy_threads_;

	mutable std::mutex  mutex_;

	//  �������� - ����������-������ ��task������ȡ������
	std::condition_variable idle_cond_;
	std::condition_variable task_cond_;
};

void ThreadPool::Impl::create_pool(int threads)
{
	threads_num_ = threads;
	for (int i = 0; i < threads_num_; i++) {
		auto thread = std::make_shared<stThread>();
		//  ����std::thread
		std::thread threadImpl([this, thread]() {
			while (!thread->is_stop_) {
				//  ��ȡһ��task
				auto task = this->get_task();

				//  ִ��task
				if (task)
					task->fn_();

				//  ���ûؿ��� - ���߳���Ҫ���û�������
				move_to_idle(thread);
			}
		});
		thread->thread_ = std::move(threadImpl);
		thread->thread_.detach();
		//  ���뵽���ж�����
		idle_threads_.insert(thread);
	}
}

void ThreadPool::Impl::move_to_idle(std::shared_ptr<stThread> thread)
{
	//LOG_DEBUG("move_to_idle_list");
	auto lock = std::unique_lock(mutex_);
	if (busy_threads_.size() == 0)
		return;
	//  ��ȡԭ�е�Ԫ�أ����뵽idle��
	auto e = busy_threads_.extract(thread);
	if (e) {
		idle_threads_.insert(std::move(e));
		//  ֪ͨ�ȴ��߳�
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
	//  �ȴ��������
	task_cond_.wait(lock, [this] { return !task_list_.empty(); });

	//  �ȴ�����
	if (task_list_.empty()) {
		return nullptr;
	}

	// ����һ������
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

	//  �ȴ������߳�
	idle_cond_.wait(lock, [this] { return !idle_threads_.empty(); });

	//  ��ȡһ�������߳�
	auto it = idle_threads_.begin();
	auto& idle_thread = *it;
	//  �ӿ����б����Ƴ�
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
	impl_->idle_cond_.notify_all(); // ���������߳�ִ��
	for (auto& thread : impl_->idle_threads_) {
		if (thread->thread_.joinable())
			thread->thread_.join(); // �ȴ���������� ǰ�᣺�߳�һ����ִ����
	}
}

void ThreadPool::assign(std::function<void()> task)
{
	impl_->add_task(task);
	//  ��ȡ�����߳�
	auto thread = impl_->get_idle_thread();
	if (thread) {
		impl_->move_to_busy(thread);
	}
}
