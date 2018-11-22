#include "semaphore.h"

#include <iostream>

Semaphore::Semaphore(int semaphore_value) : mSem_Value(semaphore_value), mQueueSize(0) {

}

void Semaphore::Wait() {
	std::unique_lock<std::mutex> lock(mMutex_Lck);
	mSem_Value--;
	while (mSem_Value < 0) { 
		mQueueSize++; // adding him into queue
		mWait_Condition.wait(lock); // waiting for someone to signal the release
	}
}

void Semaphore::Signal() {
	std::unique_lock<std::mutex> lock(mMutex_Lck);
	mSem_Value++;
	if (mQueueSize > 0) { // someone is waiting, notify him
		mQueueSize--;
		mWait_Condition.notify_one();
	}
}