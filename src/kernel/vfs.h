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

	// All possible file descriptor attributes and their combinations
	const TFD_Attributes FD_ATTR_FREE = 0x00;
	const TFD_Attributes FD_ATTR_READ = 0x01;
	const TFD_Attributes FD_ATTR_WRITE = 0x02;
	const TFD_Attributes FD_ATTR_RW = FD_ATTR_READ | FD_ATTR_WRITE;

	static const size_t MAX_FILE_DESCRIPTORS = 2048;
	static const size_t MAX_FILES_CACHED = 1024;
	static const size_t MAX_FS_REGISTERED = 4;
	static const size_t MAX_FS_MOUNTED = 10;

	const TPath DEFAULT_WORKING_DIRECTORY = { "C" , {}, "", "C:\\" };

	// Abstract class from which concrete file systems inherit
	// Instances of inherited classes represent one file
	class IFile {
		public:
			virtual size_t Write(const char *buffer, size_t buffer_size, size_t position);
			virtual size_t Read(char *buffer, size_t buffer_size, size_t position);
			virtual bool Resize(size_t size);
			virtual void Close(const TFD_Attributes attrs);
			virtual bool Is_Available_For_Write();
			virtual size_t Get_Size();
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
			kiv_os::NFile_Attributes mAttributes;
			unsigned int mRead_count = 0;
			unsigned int mWrite_count = 0;
			std::mutex mFile_lock;
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
			virtual std::shared_ptr<IFile> Open_File(const TPath &path, kiv_os::NFile_Attributes attributes);
			virtual std::shared_ptr<IFile> Create_File(const TPath &path, kiv_os::NFile_Attributes attributes);
			virtual bool Delete_File(const TPath &path);
			std::string Get_Label();

			virtual ~IMounted_File_System();
		protected:
			std::string mLabel;
	};

	// TODO predavat funkcim working directory procesù?
	class CVirtual_File_System {
		public:
			static CVirtual_File_System &Get_Instance();
			static void Destroy();
			~CVirtual_File_System();

			/*
			 * Sys calls
			 */

			// Throws	TFd_Table_Full_Exception, TFile_Not_Found_Exception, TPermissions_Denied_Exception
			bool Open_File(std::string path, kiv_os::NFile_Attributes attributes, kiv_os::THandle &fd_index);

			// Throws	TFd_Table_Full_Exception, TFile_Not_Found_Exception, TPermissions_Denied_Exception, 
			//			TNot_Enough_Space_Exception
			bool Create_File(std::string path, kiv_os::NFile_Attributes attributes, kiv_os::THandle &fd_index);

			// Throws	TInvalid_Fd_Exception
			bool Close_File(kiv_os::THandle fd_index);

			// Throws	TFile_Not_Found_Exception, TPermission_Denied_Exception
			bool Delete_File(std::string path);

			// Throws	TInvalid_Fd_Exception, TPermission_Denied_Exception
			size_t Write_File(kiv_os::THandle fd_index, char *buffer, size_t buffer_size);

			// Throws	TInvalid_Fd_Exception, TPermission_Denied_Exception
			size_t Read_File(kiv_os::THandle fd_index, char *buffer, size_t buffer_size);

			// Throws	TInvalid_Fd_Exception, TPosition_Out_Of_Range_Exception
			bool Set_Position(kiv_os::THandle fd_index, int position, kiv_os::NFile_Seek type);

			// Throws	TInvalid_Fd_Exception, TNot_Enough_Space_Exception
			bool Set_Size(kiv_os::THandle fd_index, int position, kiv_os::NFile_Seek type);

			// Throws	TInvalid_Fd_Exception
			size_t Get_Position(kiv_os::THandle fd_index);

			// Throws	TFd_Table_Full_Exception
			void Create_Pipe(kiv_os::THandle &write_end, kiv_os::THandle &read_end);

			// Throws	File_Not_Found_Exception
			void Set_Working_Directory(char *path);
			 
			/*
			 * mounting systems
			 */
			bool Register_File_System(IFile_System *fs);
			bool Mount_File_System(std::string fs_name, std::string label, TDisk_Number = 0);

		private:
			static std::recursive_mutex mFd_lock;
			static std::mutex mRegistered_fs_lock;
			static std::mutex mMounted_fs_lock;
			static std::recursive_mutex mFiles_lock;;

			unsigned int mFd_count = 0;
			unsigned int mRegistered_fs_count = 0;
			unsigned int mMounted_fs_count = 0;
			unsigned int mCached_files_count = 0;

			std::array<TFile_Descriptor, MAX_FILE_DESCRIPTORS> mFile_descriptors;
			std::map<std::string, std::shared_ptr<IFile>> mCached_files; // absolute_path -> IFile
			std::vector<IFile_System*> mRegistered_file_systems; // Pøedìlat na mapu fs_name -> fs ?
			std::map<std::string, IMounted_File_System*> mMounted_file_systems;

			static CVirtual_File_System *instance;
			CVirtual_File_System(); 
			
			TFile_Descriptor &Get_File_Descriptor(kiv_os::THandle fd_index); // Throws TInvalid_Fd_Exception whed FD is not found
			void Put_File_Descriptor(kiv_os::THandle fd_index, std::shared_ptr<IFile> file, kiv_os::NFile_Attributes attributes);
			void Remove_File_Descriptor(kiv_os::THandle fd_index);
			kiv_os::THandle Get_Free_Fd_Index(); // Throws TFd_Table_Full_Exception when mFile_descriptors is full
			IMounted_File_System *Resolve_Mount(const TPath &normalized_path);
			TPath Create_Normalized_Path(std::string path);
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

	/*
	 * Exceptions
	 */
	struct TInvalid_Fd_Exception : public std::exception {};
	struct TFd_Table_Full_Exception : public std::exception {};
	struct TPermission_Denied_Exception : public std::exception {};
	struct TFile_Not_Found_Exception : public std::exception {};
	struct TDirectory_Not_Empty_Exception : public std::exception {};
	struct TPosition_Out_Of_Range_Exception : public std::exception {};
	struct TNot_Enough_Space_Exception : public std::exception {};
	struct TInvalid_Operation_Exception : public std::exception {};
	struct TInvalid_Path_Exception : public std::exception {};
	struct TInvalid_Mount_Exception : public std::exception {};
	struct TInternal_Error_Exception : public std::exception {};
}