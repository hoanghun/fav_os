#pragma once
#include "vfs.h"
namespace kiv_fs_proc {

	class CFile;
	class CFile_System;
	class CMount;

	class CFile : public kiv_vfs::IFile {
	public:
		CFile(std::string name, CMount *mount, kiv_os::NFile_Attributes attributes);
		virtual unsigned int Write(int position, char *buffer, size_t buffer_size) final;
		virtual unsigned int Read(int position, char *buffer, size_t buffer_size) final;

	private:
		// data specificky pro soubor tohodle filesystemu (kde se v ramdisku nachazi, ...)
	};


	class CFile_System : public kiv_vfs::IFile_System {
	public:
		CFile_System();
		virtual bool Register();
		virtual kiv_vfs::IMounted_File_System *Create_Mount(const std::string label, const kiv_vfs::TDisk_Number = 0) final;
	};


	class CMount : public kiv_vfs::IMounted_File_System {
	public:
		CMount(std::string label, kiv_vfs::TDisk_Number disk);
		virtual kiv_vfs::IFile *Open_File(std::string path, kiv_os::NFile_Attributes attributes, bool create_new) final;
		virtual bool Delete_File(std::string path) final;
		kiv_vfs::TDisk_Number Get_Disk_Number();

	private:
		kiv_vfs::TDisk_Number mDisk;
	};

}