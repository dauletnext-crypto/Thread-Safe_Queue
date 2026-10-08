#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <functional>

template <typename T>




class safe_queue {
private:
	std::queue<T> m_queue;
	std::mutex m_mutex;
	std::condition_variable m_cond;

public:

	void push(T new_value) {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue.push(new_value);
		m_cond.notify_one();
	}

	void pop(T& value) {
		std::unique_lock<std::mutex> lock(m_mutex);

		m_cond.wait(lock, [this] { return !m_queue.empty(); });

		value = m_queue.front();
		m_queue.pop();
	}
};

class thread_pool {
private:
	std::vector<std::thread> m_threads;
	safe_queue<std::function<void()>> m_task_queue;
	bool m_stop = false;

	void work() {
		while (!m_stop) {
			std::function<void()> task;

			m_task_queue.pop(task);

			if (task) {
				task();
			}
		}
	}

public:
	thread_pool() {
		unsigned int cores = std::thread::hardware_concurrency();
		if (cores == 0) cores = 4;

		for (unsigned int i = 0; i < cores; ++i) {
			m_threads.push_back(std::thread(&thread_pool::work, this));;
		}
	}

	~thread_pool() {
		m_stop = true;

		for (size_t i = 0; i < m_threads.size(); ++i) {
			m_task_queue.push(std::function<void()>());
		}

		for (auto& thread : m_threads) {
			if (thread.joinable()) {
				thread.join();
			}
		}
	}

	void submit(std::function<void()> new_task) {
		m_task_queue.push(new_task);
	}
};

void print_hello() {
	std::cout << " Task executed by thread ID: " << std::this_thread::get_id() << std::endl;
}

int main() {
	std::cout << "=== THREAD POOL APPLICATION STARTED ===" << std::endl;

	thread_pool pool;

	for (int i = 1; i <= 3; ++i) {
		std::cout << "\n--- Submitting task pair " << i << " ---" << std::endl;
		pool.submit(print_hello);
		pool.submit(print_hello);

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	std::cout << "\n=== ALL TASK SUBMITTED. CLOSING POOL ===" << std::endl;
	return 0;
}