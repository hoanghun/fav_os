#pragma once

#include <mutex>
#include <condition_variable>
#include "vfs.h"
#include "semaphore.h"

class CPipe : public kiv_vfs::IFile {
public:
	CPipe();
	virtual size_t Write(const char *buffer, size_t buffer_size, size_t position = 0) final override;
	virtual size_t Read(char *buffer, size_t buffer_size, size_t position = 0) final override;

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

kiv_vfs::IFile *Create_Pipe();


