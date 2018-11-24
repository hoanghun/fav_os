#pragma once

#include "vfs.h"

namespace kiv_fs_proc {

	class CFile;
	class CFile_System;
	class CMount;

	class CFile : public kiv_vfs::IFile {
		public:
			CFile(const kiv_vfs::TPath path, kiv_os::NFile_Attributes attributes);
			virtual size_t Write(const char *buffer, size_t buffer_size, size_t position) final override;
			virtual size_t Read(char *buffer, size_t buffer_size, size_t position) final override;
			virtual bool Is_Available_For_Write() final override;

		private:
			// data specificky pro soubor tohohle filesystemu (kde se v ramdisku nachazi, ...)
	};


	class CFile_System : public kiv_vfs::IFile_System {
		public:
			CFile_System();
			virtual kiv_vfs::IMounted_File_System *Create_Mount(const std::string label, const kiv_vfs::TDisk_Number = 0) final override;
	};


	class CMount : public kiv_vfs::IMounted_File_System {
		public:
			CMount(std::string label);
			virtual std::shared_ptr<kiv_vfs::IFile> Open_File(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes) final override;
			virtual std::shared_ptr<kiv_vfs::IFile> Create_File(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes) final override;
			virtual bool Delete_File(const kiv_vfs::TPath &path) final override;
			kiv_vfs::TDisk_Number Get_Disk_Number();

		private:
			kiv_vfs::TDisk_Number mDisk;
	};

}