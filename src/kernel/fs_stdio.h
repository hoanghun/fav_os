#pragma once

#include "vfs.h"

namespace kiv_fs_stdio {

	class CFile;
	class CFile_System;
	class CMount;

	class CFile : public kiv_vfs::IFile {
	public:
		CFile(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes);
		virtual size_t Write(const char *buffer, size_t buffer_size, size_t position = 0) final override;
		virtual size_t Read(char *buffer, size_t buffer_size, size_t position = 0) final override;
		virtual bool Is_Available_For_Write() final override;

	private:
		
	};


	class CFile_System : public kiv_vfs::IFile_System {
	public:
		CFile_System(std::string name);
		virtual bool Register() final override;
		virtual kiv_vfs::IMounted_File_System *Create_Mount(const std::string label, const kiv_vfs::TDisk_Number = 0) final override;
	};


	class CMount : public kiv_vfs::IMounted_File_System {
	public:
		CMount(std::string label);
		std::shared_ptr<kiv_vfs::IFile> Open_File(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes) override;
	private:
		std::map<std::string, std::shared_ptr<kiv_vfs::IFile>> mStdioFiles;
	};

}