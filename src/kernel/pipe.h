#pragma once

#include <mutex>
#include <condition_variable>
#include "vfs.h"

namespace kiv_pipe {
	class CPipe : public kiv_vfs::IFile {
	public:
		virtual size_t Write(const char *buffer, size_t buffer_size, size_t position = 0) final override;
		virtual size_t Read(char *buffer, size_t buffer_size, size_t position = 0) final override;
	private:
		std::mutex mEmpty_lck;
		std::mutex mFull_lck;
		std::condition_variable mEmpty_cv;
		std::condition_variable mFull_cv;
	};
}


