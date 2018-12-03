#include <iostream>

#include "vfs.h"
#include "pipe.h"
#include "process.h"

namespace kiv_vfs {
#pragma region File

	// Default implementations (concrete filesystem can override those methods)
	size_t IFile::Write(const char *buffer, size_t buffer_size, size_t position) {
		return 0;
	}
	size_t IFile::Read(char *buffer, size_t buffer_size, size_t position) {
		return 0;
	}
	bool IFile::Resize(size_t size) {
		return false;
	}
	bool IFile::Is_Available_For_Write() {
		return true;
	}
	size_t IFile::Get_Size() {
		return 0;
	}
	bool IFile::Is_Empty() {
		return false;
	}
	void IFile::Close(const TFD_Attributes attrs) {
		return;
	}

	void IFile::Increase_Write_Count() {
		std::unique_lock<std::mutex> lock(mFile_lock);
		mWrite_count++;
	}

	void IFile::Decrease_Write_Count() {
		std::unique_lock<std::mutex> lock(mFile_lock);
		mWrite_count--;
	}

	void IFile::Increase_Read_Count() {
		std::unique_lock<std::mutex> lock(mFile_lock);
		mRead_count++;
	}

	void IFile::Decrease_Read_Count() {
		std::unique_lock<std::mutex> lock(mFile_lock);
		mRead_count--;
	}

	TPath IFile::Get_Path() {
		return mPath;
	}

	unsigned int IFile::Get_Write_Count() {
		return mWrite_count;
	}

	unsigned int IFile::Get_Read_Count() {
		return mRead_count;
	}

	bool IFile::Is_Opened() {
		std::unique_lock<std::mutex> lock(mFile_lock);
		return (Get_Write_Count() + Get_Read_Count() != 0);
	}

	bool IFile::Is_Directory() {
		return (mAttributes == kiv_os::NFile_Attributes::Directory);
	}

	kiv_os::NFile_Attributes IFile::Get_Attributes() {
		return mAttributes;
	}


	IFile::~IFile() {

	}

#pragma endregion


#pragma region File system

	IFile_System::~IFile_System() {
	}

	std::string IFile_System::Get_Name() {
		return mName;
	}

#pragma endregion


#pragma region Mounted file system
	std::string IMounted_File_System::Get_Label() {
		return mLabel;
	}

	IMounted_File_System::~IMounted_File_System() {

	}

	// Default implementations (concrete filesystem can override those methods)
	std::shared_ptr<IFile> IMounted_File_System::Open_File(const TPath &path, kiv_os::NFile_Attributes attributes) {
		return nullptr;
	}
	std::shared_ptr<IFile> IMounted_File_System::Create_File(const TPath &path, kiv_os::NFile_Attributes attributes) {
		return nullptr;
	}
	bool IMounted_File_System::Delete_File(const TPath &path) {
		return false;
	}

#pragma endregion


#pragma region Virtual file system
	std::recursive_mutex CVirtual_File_System::mFd_lock;
	std::mutex CVirtual_File_System::mRegistered_fs_lock;
	std::mutex CVirtual_File_System::mMounted_fs_lock;
	std::recursive_mutex CVirtual_File_System::mFiles_lock;

	CVirtual_File_System *CVirtual_File_System::instance;

	CVirtual_File_System::CVirtual_File_System() : mFd_count(0) {
	}

	CVirtual_File_System::~CVirtual_File_System() {
		Unregister_All();
		Unmount_All();
	}

	void CVirtual_File_System::Destroy() {
		delete instance;
	}

	CVirtual_File_System &CVirtual_File_System::Get_Instance() {
		if (instance == nullptr) {
			instance = new CVirtual_File_System();
		}
		return *instance;
	}

	bool CVirtual_File_System::Register_File_System(IFile_System *fs) {
		std::unique_lock<std::mutex> lock(mRegistered_fs_lock);

		if (mRegistered_fs_count == MAX_FS_REGISTERED) {
			return false;
		}
		
		for (auto &reg_file_system : mRegistered_file_systems) {
			if (fs->Get_Name() == reg_file_system->Get_Name()) {
				delete fs;
				return false;
			}
		}

		mRegistered_file_systems.push_back(fs);
		mRegistered_fs_count++;

		return true;
	}

	bool CVirtual_File_System::Mount_File_System(std::string fs_name, std::string label, TDisk_Number disk) {
		std::unique_lock<std::mutex> lock(mMounted_fs_lock);

		if (mMounted_fs_count == MAX_FS_MOUNTED) {
			return false;
		}

		for (auto fs : mRegistered_file_systems) {
			if (fs->Get_Name() == fs_name) {
				IMounted_File_System *mount = fs->Create_Mount(label, disk);
				mMounted_file_systems.insert(std::make_pair(label, mount));
				mMounted_fs_count++;
				return true;
			}
		}

		return false;
	}

	void CVirtual_File_System::Unregister_All() {
		for (auto fs : mRegistered_file_systems) {
			delete fs;
		}
	}

	void CVirtual_File_System::Unmount_All() {
		for (auto mount : mMounted_file_systems) {
			delete mount.second;
		}
	}

	kiv_os::NOS_Error CVirtual_File_System::Open_File(std::string path, kiv_os::NFile_Attributes attributes, kiv_os::THandle &fd_index) {
		kiv_os::THandle free_fd = Get_Free_Fd_Index();

		if (free_fd == kiv_os::Invalid_Handle) {
			return kiv_os::NOS_Error::Out_Of_Memory;
		}

		auto normalized_path = Create_Normalized_Path(path);

		std::shared_ptr<IFile> file;

		// File is cached
		if (Is_File_Cached(normalized_path)) {
			file = Get_Cached_File(normalized_path);
		}

		// File is not cached -> resolve file and cache file
		else {
			auto mount = Resolve_Mount(normalized_path);
			
			if (!mount) {
				return kiv_os::NOS_Error::File_Not_Found;
			}

			file = mount->Open_File(normalized_path, attributes);
			if (!file) {
				Free_File_Descriptor(free_fd);
				return kiv_os::NOS_Error::File_Not_Found;
			}
			else if ((file->Get_Attributes() == kiv_os::NFile_Attributes::Read_Only) && attributes != kiv_os::NFile_Attributes::Read_Only) {
				Free_File_Descriptor(free_fd);
				return kiv_os::NOS_Error::Permission_Denied;
			}
			else if ((attributes == kiv_os::NFile_Attributes::Directory) && (file->Get_Attributes() != kiv_os::NFile_Attributes::Directory)) {
				Free_File_Descriptor(free_fd);
				return kiv_os::NOS_Error::File_Not_Found;
			}
			Cache_File(file);
		}

		fd_index = free_fd;
		Put_File_Descriptor(free_fd, file, attributes);

		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error CVirtual_File_System::Create_File(std::string path, kiv_os::NFile_Attributes attributes, kiv_os::THandle &fd_index) {
		fd_index = Get_Free_Fd_Index();
		auto normalized_path = Create_Normalized_Path(path);
		auto mount = Resolve_Mount(normalized_path);

		if (!mount) {
			return kiv_os::NOS_Error::File_Not_Found;
		}

		// File is cached
		if (Is_File_Cached(normalized_path)) {
			auto file = Get_Cached_File(normalized_path);

			// File is opened -> cannot override this file
			if (file->Is_Opened()) {
				Free_File_Descriptor(fd_index);
				return kiv_os::NOS_Error::Permission_Denied;
			}

			// File is not opened -> can override this file
			else {
				Decache_File(file);
			}
		}

		auto file = mount->Create_File(normalized_path, attributes);
		if (!file) {
			Free_File_Descriptor(fd_index);
			return kiv_os::NOS_Error::Not_Enough_Disk_Space;
		}

		Put_File_Descriptor(fd_index, file, attributes);

		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error CVirtual_File_System::Close_File(kiv_os::THandle fd_index) {
		TFile_Descriptor *file_desc = Get_File_Descriptor(fd_index);
		
		if (file_desc->file) {
			file_desc->file->Close(file_desc->attributes);

			if (file_desc->file->Get_Read_Count() == 0 && file_desc->file->Get_Write_Count() == 0) {
				Decache_File(file_desc->file);
			}
		}
		else {
			return kiv_os::NOS_Error::File_Not_Found;
		}

		Free_File_Descriptor(fd_index);
		
		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error CVirtual_File_System::Delete_File(std::string path) {
		auto normalized_path = Create_Normalized_Path(path); 

		// File is cached
		if (Is_File_Cached(normalized_path)) {
			auto file = Get_Cached_File(normalized_path);

			// File is opened
			if (file->Is_Opened()) {
				return kiv_os::NOS_Error::Permission_Denied;
			}

			// File is not opened
			else {
				Decache_File(file);
			}
		}

		auto mount = Resolve_Mount(normalized_path);

		if (!mount) {
			return kiv_os::NOS_Error::File_Not_Found;
		}

		auto file = mount->Open_File(normalized_path, (kiv_os::NFile_Attributes)0);
		if (!file) {
			return kiv_os::NOS_Error::File_Not_Found;
		}

		// Cannot delete a non-empty directory
		if (file->Is_Directory() && !file->Is_Empty()) {
			return kiv_os::NOS_Error::Directory_Not_Empty;
		}

		if (!mount->Delete_File(normalized_path)) {
			return kiv_os::NOS_Error::File_Not_Found;
		}

		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error CVirtual_File_System::Write_File(kiv_os::THandle fd_index, char *buffer, size_t buffer_size, size_t &written) {
		TFile_Descriptor *file_desc = Get_File_Descriptor(fd_index);

		if (!file_desc) {
			return kiv_os::NOS_Error::File_Not_Found;
		}
		
		if (!(file_desc->attributes & FD_ATTR_WRITE)) {
			return kiv_os::NOS_Error::Permission_Denied;
		}

		size_t bytes_written = file_desc->file->Write(buffer, buffer_size, file_desc->position);
		file_desc->position += bytes_written;

		written = bytes_written;

		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error CVirtual_File_System::Read_File(kiv_os::THandle fd_index, char *buffer, size_t buffer_size, size_t &read) {
		TFile_Descriptor *file_desc = Get_File_Descriptor(fd_index);
		
		if (!file_desc) {
			return kiv_os::NOS_Error::File_Not_Found;
		}

		if (!(file_desc->attributes & FD_ATTR_READ)) {
			return kiv_os::NOS_Error::Permission_Denied;
		}

		size_t bytes_read = file_desc->file->Read(buffer, buffer_size, file_desc->position);
		file_desc->position += bytes_read;

		read = bytes_read;

		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error CVirtual_File_System::Set_Position(kiv_os::THandle fd_index, int position, kiv_os::NFile_Seek type) {
		TFile_Descriptor *file_desc = Get_File_Descriptor(fd_index);

		if (!file_desc) {
			return kiv_os::NOS_Error::File_Not_Found;
		}

		size_t tmp_pos = Calculate_Position(*file_desc, position, type);

		if (tmp_pos > file_desc->file->Get_Size() || tmp_pos < 0) {
			return kiv_os::NOS_Error::IO_Error;
		}

		file_desc->position = tmp_pos;
		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error CVirtual_File_System::Set_Size(kiv_os::THandle fd_index, int position, kiv_os::NFile_Seek type) {
		TFile_Descriptor *file_desc = Get_File_Descriptor(fd_index);

		if (!file_desc) {
			return kiv_os::NOS_Error::File_Not_Found;
		}

		size_t actual_position = Calculate_Position(*file_desc, position, type);

		if (!file_desc->file->Resize(actual_position)) {
			return kiv_os::NOS_Error::Not_Enough_Disk_Space;
		}

		file_desc->position = actual_position;

		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error CVirtual_File_System::Get_Position(kiv_os::THandle fd_index, size_t &position) {
		TFile_Descriptor *file_desc = Get_File_Descriptor(fd_index);
		
		if (!file_desc) {
			return kiv_os::NOS_Error::File_Not_Found;
		}

		position =  file_desc->position;

		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error CVirtual_File_System::Create_Pipe(kiv_os::THandle &write_end, kiv_os::THandle &read_end) {
		std::shared_ptr<kiv_vfs::IFile> pipe = std::make_shared<CPipe>();
		
		write_end = Get_Free_Fd_Index();
		Put_File_Descriptor(write_end, pipe, kiv_os::NFile_Attributes::System_File);
		read_end = Get_Free_Fd_Index();
		Put_File_Descriptor(read_end, pipe, kiv_os::NFile_Attributes::Read_Only);

		return kiv_os::NOS_Error::Success;
	}

	kiv_os::NOS_Error CVirtual_File_System::Set_Working_Directory(char *path) {
		TPath normalized_path = Create_Normalized_Path(path);
		auto mount = Resolve_Mount(normalized_path);

		if (!mount) {
			return kiv_os::NOS_Error::File_Not_Found;
		}

		if (!mount->Open_File(normalized_path, kiv_os::NFile_Attributes::Directory)) { // TODO correct attrs?
			// TODO prevent from deleting working directory?
			return kiv_os::NOS_Error::File_Not_Found;
		}
	
		kiv_process::CProcess_Manager::Get_Instance().Set_Working_Directory(normalized_path);

		return kiv_os::NOS_Error::Success;
	}

	// ====================
	// ===== PRIVATE ======
	// ====================

	TFile_Descriptor *CVirtual_File_System::Get_File_Descriptor(kiv_os::THandle fd_index) {
		std::unique_lock<std::recursive_mutex> lock(mFd_lock);

		if (fd_index > MAX_FILE_DESCRIPTORS) {
			return nullptr;
		}

		return &mFile_descriptors[fd_index];
	}

	void CVirtual_File_System::Put_File_Descriptor(kiv_os::THandle fd_index, std::shared_ptr<IFile> file, kiv_os::NFile_Attributes attributes) {
		std::unique_lock<std::recursive_mutex> lock(mFd_lock);

		TFile_Descriptor &file_desc = mFile_descriptors[fd_index];

		file_desc.position = 0;
		file_desc.file = file;

		if (attributes == kiv_os::NFile_Attributes::Read_Only) {
			file_desc.attributes = FD_ATTR_READ;
		}
		else {
			file_desc.attributes = FD_ATTR_RW;
		}

		mFd_count++;
		Increase_File_References(file_desc);
	}

	void CVirtual_File_System::Free_File_Descriptor(kiv_os::THandle fd_index) {
		std::unique_lock<std::recursive_mutex> lock(mFd_lock);

		TFile_Descriptor &file_desc = mFile_descriptors[fd_index];

		mFd_count--;
		if (file_desc.file != nullptr) {
			Decrease_File_References(file_desc);
		}

		file_desc.attributes = FD_ATTR_FREE;
		file_desc.file = nullptr;
	}

	kiv_os::THandle CVirtual_File_System::Get_Free_Fd_Index() {
		std::unique_lock<std::recursive_mutex> lock(mFd_lock);

		if (mFd_count == MAX_FILE_DESCRIPTORS) {
			return kiv_os::Invalid_Handle;
		}

		for (kiv_os::THandle i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
			if (mFile_descriptors[i].file == nullptr && mFile_descriptors[i].attributes == FD_ATTR_FREE) { 
				mFile_descriptors[i].attributes = FD_ATTR_RESERVED;
				return i;
			}
		}

		return kiv_os::Invalid_Handle;
	}

	IMounted_File_System *CVirtual_File_System::Resolve_Mount(const TPath &normalized_path) {
		std::unique_lock<std::mutex> lock(mMounted_fs_lock);

		if (mMounted_file_systems.find(normalized_path.mount) == mMounted_file_systems.end()) {
			return nullptr;
		}
		return mMounted_file_systems.at(normalized_path.mount);
	}

	bool CVirtual_File_System::Is_File_Cached(const TPath& path) {
		std::unique_lock<std::recursive_mutex> lock(mFiles_lock);

		return (mCached_files.find(path.absolute_path) != mCached_files.end());
	}

	void CVirtual_File_System::Cache_File(std::shared_ptr<IFile> &file) {
		std::unique_lock<std::recursive_mutex> lock(mFiles_lock);

		mCached_files.insert(std::pair<std::string, std::shared_ptr<IFile>>(file->Get_Path().absolute_path, file));
	}

	void CVirtual_File_System::Decache_File(std::shared_ptr<IFile> &file) {
		std::unique_lock<std::recursive_mutex> lock(mFiles_lock);

		auto path = file->Get_Path();
		if (!Is_File_Cached(file->Get_Path())) {
			return;
		}

		mCached_files.erase(mCached_files.find(file->Get_Path().absolute_path));
	}

	std::shared_ptr<IFile> CVirtual_File_System::Get_Cached_File(const TPath &path) {
		std::unique_lock<std::recursive_mutex> lock(mFiles_lock);

		return mCached_files.find(path.absolute_path)->second;
	}

	size_t CVirtual_File_System::Calculate_Position(const TFile_Descriptor &file_desc, int position, kiv_os::NFile_Seek type) {
		size_t tmp_pos;
		switch (type) {
			case kiv_os::NFile_Seek::Beginning:
				tmp_pos = 0;
				break;

			case kiv_os::NFile_Seek::Current:
				tmp_pos = file_desc.position;
				break;

			case kiv_os::NFile_Seek::End:
				tmp_pos = file_desc.file->Get_Size() - 1; // TODO check if correct
				break;
		}

		return (tmp_pos + position);
	}

	void CVirtual_File_System::Increase_File_References(TFile_Descriptor &file_desc) {
		if (file_desc.attributes & FD_ATTR_READ) {
			file_desc.file->Increase_Read_Count();
		}
		if (file_desc.attributes & FD_ATTR_WRITE) {
			file_desc.file->Increase_Write_Count();
		}
	}

	void CVirtual_File_System::Decrease_File_References(const TFile_Descriptor &file_desc) {
		if (file_desc.attributes & FD_ATTR_READ) {
			file_desc.file->Decrease_Read_Count();
		}
		if (file_desc.attributes & FD_ATTR_WRITE) {
			file_desc.file->Decrease_Write_Count();
		}
	}

	std::vector<std::string> Split(std::string str, std::string delimiter) {
		std::vector<std::string> splitted;

		size_t start = 0U;
		size_t end = str.find(delimiter);
		while (end != std::string::npos) {
			splitted.push_back(str.substr(start, end - start));
			start = end + delimiter.length();
			end = str.find(delimiter, start);
		}

		splitted.push_back(str.substr(start, end));

		return splitted;
	};

	TPath CVirtual_File_System::Create_Normalized_Path(std::string path) {
		const std::string path_delimiter = "\\";
		const std::string mount_delimiter = ":" + path_delimiter;
		const size_t mount_max_size = 6;

		TPath result;

		// Normalize slashes
		std::replace(path.begin(), path.end(), '/', '\\');

		std::vector<std::string> splitted_by_mount = Split(path, mount_delimiter);

		// Absolute path
		if (splitted_by_mount.size() == 2) {
			result.mount = splitted_by_mount.at(0);
			result.path = Split(splitted_by_mount.at(1), path_delimiter);
		}

		// Relative path
		else if (splitted_by_mount.size() == 1) {
			TPath working_dir;
			kiv_process::CProcess_Manager::Get_Instance().Get_Working_Directory(&working_dir);
			working_dir.path.push_back(working_dir.file);

			auto path_splitted = Split(splitted_by_mount.at(0), path_delimiter);

			result.mount = working_dir.mount;

			// Concatenate working directory and path
			result.path.reserve(working_dir.path.size() + path_splitted.size());
			result.path.insert(result.path.end(), working_dir.path.begin(), working_dir.path.end());
			result.path.insert(result.path.end(), path_splitted.begin(), path_splitted.end());
		}

		// Wrong format (multiple mount separators)
		else {
			throw TFile_Not_Found_Exception();
		}

		// Handle dots and empty parts
		auto itr = result.path.begin();
		while (itr != result.path.end()) {
			if (*itr == "") {
				itr = result.path.erase(itr);
				continue;
			}
			else if (*itr == "..") {
				// ".." on the root -> do nothing
				if (itr == result.path.begin()) {
					itr = result.path.erase(itr);
				}

				// ".." is at the end
				else if ((itr + 1) == result.path.end()) {
					itr = result.path.erase(itr);
					if (!result.path.empty()) {
						itr = result.path.erase(result.path.end() - 1);
					}
				}

				else {
					itr = result.path.erase(itr - 1, itr + 1);
				}
				continue;
			}
			else if (*itr == ".") {
				itr = result.path.erase(itr);
				continue;
			}

			itr++;
		}

		// Remove filename from 'result->path' and insert it into a 'result->file'
		if (!result.path.empty()) {
			result.file = result.path.back();
			result.path.pop_back();
		}

		// Create absolute path
		result.absolute_path += result.mount + mount_delimiter; // 'C:\'
		for (std::vector<std::string>::iterator it = result.path.begin(); it != result.path.end(); ++it) {
			result.absolute_path += *it + path_delimiter;
		}
		result.absolute_path += result.file;

		return result;
	}

#pragma endregion

}
