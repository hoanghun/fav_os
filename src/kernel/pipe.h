#pragma once

#include <mutex>
#include <condition_variable>

#include "semaphore.h"
#include "vfs.h"

class CPipe : public kiv_vfs::IFile {
public:
	CPipe();
	virtual kiv_os::NOS_Error Write(const char *buffer, size_t buffer_size, size_t position, size_t &written) final override;
	virtual kiv_os::NOS_Error Read(char *buffer, size_t buffer_size, size_t position, size_t &read) final override;
	void Close(const kiv_vfs::TFD_Attributes attrs) override;

private:
	static const size_t BUFFER_SIZE = 0xFFFF;
	/*
	 * ring buffer
	 */
	char mBuffer[BUFFER_SIZE];
	size_t mRead_Index; // tail
	size_t mWrite_Index; // head

	bool mReader_Closed;
	bool mWriter_Closed;

	Semaphore mEmptyCount;
	Semaphore mFillCount;
};

