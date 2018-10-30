#pragma once
#include <string>
#include <vector>
#include "file_table.h"
#include "../api/api.h"
#include <cstdint>
#include <map>
#include <array>

namespace kiv_vfs {
	using TDisk_Number = std::uint8_t;
	using TFD_Attributes = std::uint8_t;

	// All possible file descriptor attributes and their combinations
	const TFD_Attributes FD_ATTR_READ = 0x00;
	const TFD_Attributes FD_ATTR_WRITE = 0x01;
	const TFD_Attributes FD_ATTR_RW = FD_ATTR_READ | FD_ATTR_WRITE;

	static const size_t MAX_FILE_DESCRIPTORS = 1024;
	static const size_t MAX_FILES_OPENED = 1024;
	static const size_t MAX_FS_REGISTERED = 4;
	static const size_t MAX_FS_MOUNTED = 10;

	class IFile;
	class IMounted_File_System;
	class IFile_System;
	class CVirtual_File_System;

	// Opened file
	struct TFile_Descriptor {
		unsigned int position;
		IFile *file;
		TFD_Attributes attributes;
	};

	// Abstract class from which concrete file systems inherit
	// Instances of inherited classes represent one file
	class IFile {
	public:
		virtual unsigned int Write(int position, char *buffer, size_t buffer_size) = 0;
		virtual unsigned int Read(int position, char *buffer, size_t buffer_size) = 0;

		void Increase_Reference_Count();
		void Decrease_Reference_Count();

		std::string Get_Name();
		IMounted_File_System *Get_Mount();
		unsigned int Get_Reference_Count();
		kiv_os::NFile_Attributes Get_Attributes();
		bool Is_Available_For_Writting();

	protected:
		std::string mName;
		IMounted_File_System *mMount;
		kiv_os::NFile_Attributes mAttributes;
		unsigned int mReference_count;
		bool mAvailable_For_Writting;
	};

	// Abstract class from which concrete file systems inherit
	// Instances of inherited classes represent one registered file system 
	class IFile_System {
	public:
		virtual bool Register() = 0;
		virtual IMounted_File_System *Create_Mount(std::string label, TDisk_Number = 0) = 0;
		std::string Get_Name();

	protected:
		std::string mName;
	};

	// Class representing one mounted file system (file system's 'Create_Mount' returns instance of class that inherits from this class)
	class IMounted_File_System {
	public:
		virtual IFile *Open_File();
		virtual bool Delete_File();
		std::string Get_Label();

	protected:
		std::string mLabel;
	};

	class CVirtual_File_System {
	public:
		static CVirtual_File_System &Get_Instance();
		static void Destroy();

		bool Open_File(std::string path, kiv_os::NFile_Attributes attributes, bool create_new);
		bool Close_File(kiv_os::THandle fd);
		bool Delete_File(std::string path);
		unsigned int Write_File(kiv_os::THandle fd, char *buffer, size_t buffer_size);
		unsigned int Read_File(kiv_os::THandle fd, char *buffer, size_t buffer_size);
		bool Set_Position(kiv_os::THandle fd, unsigned int position, kiv_os::NFile_Seek type);
		bool Set_Size(kiv_os::THandle fd, unsigned int position, kiv_os::NFile_Seek type);
		int Get_Position(kiv_os::THandle fd);
		bool Create_Pipe(kiv_os::THandle &write_end, kiv_os::THandle &read_end);

		// TODO Set working directory, get working directory?

		bool Register_File_System(IFile_System &fs);
		bool Mount_File_System(std::string fs_name, std::string label, TDisk_Number = 0);

	private:
		CVirtual_File_System(); 
		static CVirtual_File_System *instance;

		unsigned int mFd_count = 0;
		unsigned int mOpened_files_count = 0;
		unsigned int mRegistered_fs_count = 0;
		unsigned int mMounted_fs_count = 0;

		std::array<TFile_Descriptor *, MAX_FILE_DESCRIPTORS> file_descriptors{ nullptr };
		std::map<kiv_os::THandle, IFile *> opened_files;

		std::array<IFile_System *, MAX_FS_REGISTERED> registered_file_systems{ nullptr }; // Pøedìlat na mapu fs_name -> fs
		std::array<IMounted_File_System *, MAX_FS_MOUNTED> mounted_file_systems{ nullptr };
	};
}