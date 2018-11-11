#pragma once
#include <mutex>

#include "vfs.h"
#include "../api/api.h"

namespace kiv_fs_fat {

	class CFile;
	class CFile_System;
	class CMount;

	const size_t FAT_SECTOR_SIZE = 512;
	const char *FS_NAME = "fat";

	struct TSuperblock {
		char name[4]; // fat + '\n'
		kiv_hal::TDrive_Parameters disk_params;
		size_t sectors_per_cluster;

		// fat table start, number of fat entries
		// data start
	};

	struct TFAT_Entry {
		size_t order;
		TFAT_Entry &next;
	};

	struct TFAT_Dir_Entry {
		// TODO
	};

	class CFile : public kiv_vfs::IFile {
		public:
			CFile(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes, kiv_vfs::TDisk_Number disk_number, TSuperblock &sb);
			virtual size_t Write(const char *buffer, size_t buffer_size, size_t position) final override;
			virtual size_t Read(char *buffer, size_t buffer_size, size_t position) final override;
			virtual bool Is_Available_For_Write() final override;
		private:
			kiv_vfs::TDisk_Number mDisk_number;
			TSuperblock &mSuperblock;
	};


	class CFile_System : public kiv_vfs::IFile_System {
		public:
			CFile_System();
			virtual bool Register() final override;
			virtual kiv_vfs::IMounted_File_System *Create_Mount(const std::string label, const kiv_vfs::TDisk_Number disk_number) final override;
	};


	class CMount : public kiv_vfs::IMounted_File_System {
		public:
			CMount(std::string label, kiv_vfs::TDisk_Number disk_number);
			virtual std::shared_ptr<kiv_vfs::IFile> Open_File(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes) final override;
			virtual std::shared_ptr<kiv_vfs::IFile> Create_File(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes) final override;
			virtual bool Delete_File(const kiv_vfs::TPath &path) final override;
		private:
			kiv_vfs::TDisk_Number mDisk_Number;
			TSuperblock mSuperblock;

			bool Load_Superblock();
			bool Chech_Superblock();
			bool Format_Disk();
	};

}