#pragma once
#include <string>
#include <vector>
#include "../api/api.h"
#include <cstdint>
#include <map>
#include <array>

namespace kiv_vfs {
	using TDisk_Number = std::uint8_t;
	using TFD_Attributes = std::uint8_t;

	// All possible file descriptor attributes and their combinations
	const TFD_Attributes FD_ATTR_READ = 0x01;
	const TFD_Attributes FD_ATTR_WRITE = 0x02;
	const TFD_Attributes FD_ATTR_RW = FD_ATTR_READ | FD_ATTR_WRITE;

	static const size_t MAX_FILE_DESCRIPTORS = 2048;
	static const size_t MAX_FILES_CACHED = 1024;
	static const size_t MAX_FS_REGISTERED = 4;
	static const size_t MAX_FS_MOUNTED = 10;

	class IFile;
	class IMounted_File_System;
	class IFile_System;
	class CVirtual_File_System;

	// Opened file
	struct TFile_Descriptor {
		size_t position;
		std::shared_ptr<IFile> file;
		TFD_Attributes attributes;
	};

	// Format: mount:/path[0]/path[1]/path[2]/file
	struct TPath {
		std::string mount;
		std::vector<std::string> path;
		std::string file;
		std::string absolute_path;
	};

	// Abstract class from which concrete file systems inherit
	// Instances of inherited classes represent one file
	class IFile {
		public:
			virtual size_t Write(const char *buffer, size_t buffer_size, int position) = 0;
			virtual size_t Read(char *buffer, size_t buffer_size, int position) = 0;
			virtual bool Is_Available_For_Write() = 0;
			virtual size_t Get_Size();

			void Increase_Write_Count();
			void Decrease_Write_Count();
			void Increase_Read_Count();
			void Decrease_Read_Count();

			TPath &Get_Path();
			unsigned int Get_Write_Count();
			unsigned int Get_Read_Count();
			bool Is_Opened();
			kiv_os::NFile_Attributes Get_Attributes();

		protected:
			TPath mPath;
			kiv_os::NFile_Attributes mAttributes;
			unsigned int mRead_count;
			unsigned int mWrite_count;
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
			virtual std::shared_ptr<IFile> Open_File(const TPath &path, kiv_os::NFile_Attributes attributes);
			virtual std::shared_ptr<IFile> Create_File(const TPath &path, kiv_os::NFile_Attributes attributes);
			virtual bool Delete_File(const TPath &path);
			std::string Get_Label();
		protected:
			std::string mLabel;
	};

	// TODO predavat funkcim working directory procesù?
	class CVirtual_File_System {
		public:
			static CVirtual_File_System &Get_Instance();
			static void Destroy();

			/*
			 * Sys calls
			 */
			// Throws TFd_Table_Full_Exception, TFile_Not_Found_Exception, TInvalid_Mount_Exception
			bool Open_File(std::string path, kiv_os::NFile_Attributes attributes, kiv_os::THandle &fd_index);
			bool Create_File(std::string path, kiv_os::NFile_Attributes attributes, kiv_os::THandle &fd_index);
			bool Close_File(kiv_os::THandle fd_index);
			// Throws TInvalid_Path_Exception, TFile_Not_Found_Exception
			bool Delete_File(std::string path);
			unsigned int Write_File(kiv_os::THandle fd_index, char *buffer, size_t buffer_size);
			unsigned int Read_File(kiv_os::THandle fd_index, char *buffer, size_t buffer_size);
			// Throws TPosition_Out_Of_Range_Exception
			bool Set_Position(kiv_os::THandle fd_index, int position, kiv_os::NFile_Seek type);
			bool Set_Size(kiv_os::THandle fd_index, int position, kiv_os::NFile_Seek type);
			size_t Get_Position(kiv_os::THandle fd_index);
			bool Create_Pipe(kiv_os::THandle &write_end, kiv_os::THandle &read_end);
			// TODO Set working directory, get working directory?
			 
			/*
			 * mounting systems
			 */
			bool Register_File_System(IFile_System &fs);
			bool Mount_File_System(std::string fs_name, std::string label, TDisk_Number = 0);
		// TODO Locks
		private:
			CVirtual_File_System(); 
			static CVirtual_File_System *instance;

			unsigned int mFd_count = 0;
			unsigned int mCached_files_count = 0;
			unsigned int mRegistered_fs_count = 0;
			unsigned int mMounted_fs_count = 0;

			std::array<TFile_Descriptor, MAX_FILE_DESCRIPTORS> mFile_descriptors;
			std::map<std::string, std::shared_ptr<IFile>> mCached_files; // absolute_path -> IFile

			std::array<std::shared_ptr<IFile_System>, MAX_FS_REGISTERED> mRegistered_file_systems{ nullptr }; // Pøedìlat na mapu fs_name -> fs ?
			std::array<std::shared_ptr<IMounted_File_System>, MAX_FS_MOUNTED> mMounted_file_systems{ nullptr };

			TFile_Descriptor Create_File_Descriptor(std::shared_ptr<IFile> file, kiv_os::NFile_Attributes attributes);
			// Throws TInvalid_Fd_Exception whed FD is not found
			TFile_Descriptor& Get_File_Descriptor(kiv_os::THandle fd_index);
			void Put_File_Descriptor(kiv_os::THandle fd_index, TFile_Descriptor &file_desc);
			// Throws TFd_Table_Full_Exception when mFile_descriptors is full
			kiv_os::THandle Get_Free_Fd_Index();
			std::shared_ptr<IMounted_File_System> Resolve_Mount(const TPath &normalized_path);
			TPath Create_Normalized_Path(std::string path);
			void Increase_File_References(std::shared_ptr<IFile> &file, kiv_os::NFile_Attributes attributes);
			void Decrease_File_References(const TFile_Descriptor &file_desc);
			bool Is_File_Cached(const TPath &path);
			void Cache_File(std::shared_ptr<IFile> &file);
			void Decache_File(std::shared_ptr<IFile> &file);
			std::shared_ptr<IFile> Get_Cached_File(const TPath &path);
	};

	/*
	 * Exceptions
	 */
	struct TInvalid_Fd_Exception : public std::exception {};
	struct TFd_Table_Full_Exception : public std::exception {};
	
	struct TRead_Only : public std::exception{};
	struct TFile_Not_Found_Exception : public std::exception {};
	struct TPosition_Out_Of_Range_Exception : public std::exception {};

	struct TInvalid_Path_Exception : public std::exception {};
	struct TInvalid_Mount_Exception : public std::exception {};

	struct TInternal_Error_Exception : public std::exception {};
}