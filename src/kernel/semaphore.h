#pragma once

#include <mutex>
#include <condition_variable>

class Semaphore {
public:
	Semaphore(int semaphore_value);
	/*
	 * Semaphore P operation, has to wait if mSemValue <= 0
	 */
	void Wait();

	/*
	 * Semaphore V operation, either signals one thread waiting for lock or increases mSemValue
	 */
	void Signal();

private:
	std::condition_variable mWait_Condition;
	std::mutex mMutex_Lck;
	int mSem_Value;
	size_t mQueueSize;
};
