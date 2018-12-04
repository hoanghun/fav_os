#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include <array>
#include <mutex>

#include "../api/api.h"

namespace kiv_vfs {
	class IFile;
	class IMounted_File_System;
	class IFile_System;
	class CVirtual_File_System;

	using TDisk_Number = std::uint8_t;
	using TFD_Attributes = std::uint8_t;

	// All possible file descriptor attributes and their combinations
	const TFD_Attributes FD_ATTR_FREE = 0x00;
	const TFD_Attributes FD_ATTR_RESERVED = 0x01;
	const TFD_Attributes FD_ATTR_READ = 0x02;
	const TFD_Attributes FD_ATTR_WRITE = 0x04;
	const TFD_Attributes FD_ATTR_RW = FD_ATTR_READ | FD_ATTR_WRITE;

	static const size_t MAX_FILE_DESCRIPTORS = 2048;
	static const size_t MAX_FILES_CACHED = 1024;
	static const size_t MAX_FS_REGISTERED = 4;
	static const size_t MAX_FS_MOUNTED = 10;

	// Opened file
	struct TFile_Descriptor {
		size_t position = 0;
		std::shared_ptr<IFile> file;
		TFD_Attributes attributes = FD_ATTR_FREE;
	};

	// Format: mount:/path[0]/path[1]/path[2]/file
	struct TPath {
		std::string mount;
		std::vector<std::string> path;
		std::string file;
		std::string absolute_path;
	};

	const TPath DEFAULT_WORKING_DIRECTORY = { "C" , {}, "", "C:\\" };

	// Abstract class from which concrete file systems inherit
	// Instances of inherited classes represent one file
	class IFile {
		public:
			virtual kiv_os::NOS_Error Write(const char *buffer, size_t buffer_size, size_t position, size_t &written);
			virtual kiv_os::NOS_Error Read(char *buffer, size_t buffer_size, size_t position, size_t &read);
			virtual kiv_os::NOS_Error Resize(size_t size);
			virtual size_t Get_Size();
			virtual void Close(const TFD_Attributes attrs);
			virtual bool Is_Available_For_Write();
			virtual bool Is_Empty();

			void Increase_Write_Count();
			void Decrease_Write_Count();
			void Increase_Read_Count();
			void Decrease_Read_Count();

			TPath Get_Path();
			unsigned int Get_Write_Count();
			unsigned int Get_Read_Count();
			bool Is_Opened();
			bool Is_Directory();
			kiv_os::NFile_Attributes Get_Attributes();

			virtual ~IFile();

		protected:
			TPath mPath;
			kiv_os::NFile_Attributes mAttributes = static_cast<kiv_os::NFile_Attributes>(0);
			unsigned int mRead_count = 0;
			unsigned int mWrite_count = 0;
			std::recursive_mutex mFile_lock;
	};

	// Abstract class from which concrete file systems inherit
	// Instances of inherited classes represent one registered file system 
	class IFile_System {
		public:
			virtual ~IFile_System();
			virtual IMounted_File_System *Create_Mount(std::string label, TDisk_Number = 0) = 0;
			std::string Get_Name();

		protected:
			std::string mName;
	};

	// Class representing one mounted file system (file system's 'Create_Mount' returns instance of class that inherits from this class)
	class IMounted_File_System {
		public:
			virtual kiv_os::NOS_Error Open_File(const TPath &path, kiv_os::NFile_Attributes attributes, std::shared_ptr<IFile> &file);
			virtual kiv_os::NOS_Error Create_File(const TPath &path, kiv_os::NFile_Attributes attributes, std::shared_ptr<IFile> &file);
			virtual kiv_os::NOS_Error Delete_File(const TPath &path);
			std::string Get_Label();

			virtual ~IMounted_File_System();
		protected:
			std::string mLabel;
	};

	class CVirtual_File_System {
		public:
			static CVirtual_File_System &Get_Instance();
			static void Destroy();
			~CVirtual_File_System();

			/*
			 * Sys calls
			 */
			kiv_os::NOS_Error Open_File(std::string path, kiv_os::NFile_Attributes attributes, kiv_os::THandle &fd_index);
	
			kiv_os::NOS_Error Create_File(std::string path, kiv_os::NFile_Attributes attributes, kiv_os::THandle &fd_index);
		
			kiv_os::NOS_Error Close_File(kiv_os::THandle fd_index);

			kiv_os::NOS_Error Delete_File(std::string path);

			kiv_os::NOS_Error Write_File(kiv_os::THandle fd_index, char *buffer, size_t buffer_size, size_t &written);

			kiv_os::NOS_Error Read_File(kiv_os::THandle fd_index, char *buffer, size_t buffer_size, size_t &read);

			kiv_os::NOS_Error Set_Position(kiv_os::THandle fd_index, int position, kiv_os::NFile_Seek type);

			kiv_os::NOS_Error Set_Size(kiv_os::THandle fd_index, int position, kiv_os::NFile_Seek type);

			kiv_os::NOS_Error Get_Position(kiv_os::THandle fd_index, size_t &position);

			kiv_os::NOS_Error Create_Pipe(kiv_os::THandle &write_end, kiv_os::THandle &read_end);

			kiv_os::NOS_Error Set_Working_Directory(char *path);
			 
			/*
			 * mounting systems
			 */
			bool Register_File_System(IFile_System *fs);
			bool Mount_File_System(std::string fs_name, std::string label, TDisk_Number = 0);

		private:
			static std::recursive_mutex mFd_lock;
			static std::mutex mRegistered_fs_lock;
			static std::mutex mMounted_fs_lock;
			static std::recursive_mutex mFiles_lock;

			unsigned int mFd_count = 0;
			unsigned int mRegistered_fs_count = 0;
			unsigned int mMounted_fs_count = 0;
			unsigned int mCached_files_count = 0;

			std::array<TFile_Descriptor, MAX_FILE_DESCRIPTORS> mFile_descriptors;
			std::map<std::string, std::shared_ptr<IFile>> mCached_files; // absolute_path -> IFile
			std::vector<IFile_System*> mRegistered_file_systems;
			std::map<std::string, IMounted_File_System*> mMounted_file_systems;

			static CVirtual_File_System *instance;
			CVirtual_File_System(); 
			
			TFile_Descriptor *Get_File_Descriptor(kiv_os::THandle fd_index);
			void Put_File_Descriptor(kiv_os::THandle fd_index, std::shared_ptr<IFile> file, kiv_os::NFile_Attributes attributes);
			void Free_File_Descriptor(kiv_os::THandle fd_index);
			kiv_os::THandle Get_Free_Fd_Index(); 
			IMounted_File_System *Resolve_Mount(const TPath &normalized_path);
			bool Create_Normalized_Path(std::string path, TPath &normalized_path);
			void Increase_File_References(TFile_Descriptor &file_desc);
			void Decrease_File_References(const TFile_Descriptor &file_desc);
			bool Is_File_Cached(const TPath &path);
			void Cache_File(std::shared_ptr<IFile> &file);
			void Decache_File(std::shared_ptr<IFile> &file);
			std::shared_ptr<IFile> Get_Cached_File(const TPath &path);
			size_t Calculate_Position(const TFile_Descriptor &file_desc, int position, kiv_os::NFile_Seek type);

			void Unmount_All();
			void Unregister_All();
	};
}