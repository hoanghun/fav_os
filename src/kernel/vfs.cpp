#include "vfs.h"

namespace kiv_vfs {

#pragma region File

	// Default implementation (concrete filesystem can override this method)
	size_t IFile::Get_Size() {
		return 0;
	}

	void IFile::Increase_Write_Count() {
		mWrite_count++;
	}

	void IFile::Decrease_Write_Count() {
		mWrite_count--;
	}

	void IFile::Increase_Read_Count() {
		mRead_count++;
	}

	void IFile::Decrease_Read_Count() {
		mRead_count--;
	}

	TPath &IFile::Get_Path() {
		return mPath;
	}

	unsigned int IFile::Get_Write_Count() {
		return mWrite_count;
	}

	unsigned int IFile::Get_Read_Count() {
		return mRead_count;
	}

	bool IFile::Is_Opened() {
		return (Get_Write_Count() + Get_Read_Count() == 0);
	}

	kiv_os::NFile_Attributes IFile::Get_Attributes() {
		return mAttributes;
	}

	IFile::~IFile() {

	}

#pragma endregion


#pragma region File system

	std::string IFile_System::Get_Name() {
		return mName;
	}

	IFile_System::~IFile_System() {

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

	CVirtual_File_System *CVirtual_File_System::instance = new CVirtual_File_System();

	CVirtual_File_System::CVirtual_File_System() {
		mRegistered_file_systems.reserve(MAX_FS_REGISTERED);
	}

	void CVirtual_File_System::Destroy() {
		// TODO cleanup?
		delete instance;
	}

	CVirtual_File_System &CVirtual_File_System::Get_Instance() {
		return *instance;
	}

	bool CVirtual_File_System::Register_File_System(IFile_System &fs) {
		if (mRegistered_fs_count == MAX_FS_REGISTERED) {
			return false;
		}
		
		for (auto &reg_file_system : mRegistered_file_systems) {
			if (fs.Get_Name() == reg_file_system->Get_Name())
				return false;
		}

		mRegistered_file_systems.push_back(&fs);
		mRegistered_fs_count++;

		return true;
	}

	bool CVirtual_File_System::Mount_File_System(std::string fs_name, std::string label, TDisk_Number disk) {
		// TODO
		return false;
	}

	void CVirtual_File_System::Mount_Registered() {
		for (auto reg_file_system : mRegistered_file_systems) {
			auto mount = reg_file_system->Create_Mount(reg_file_system->Get_Name());
			mMounted_file_systems.insert(std::make_pair(reg_file_system->Get_Name(), mount)); // TODO 
		}
	}

	void CVirtual_File_System::UMount() {
		for (auto mount : mMounted_file_systems) {
			delete mount.second;
		}
	}

	bool CVirtual_File_System::Open_File(std::string path, kiv_os::NFile_Attributes attributes, kiv_os::THandle &fd_index) {
		kiv_os::THandle free_fd = Get_Free_Fd_Index(); // throws TFd_Table_Full_Exception
		auto normalized_path = Create_Normalized_Path(path); // throws TFile_Not_Found_Exception

		std::shared_ptr<IFile> file;

		// File is cached
		if (Is_File_Cached(normalized_path)) {
			file = Get_Cached_File(normalized_path);
		}

		// File is not cached -> resolve file and cache file
		else {
			auto mount = Resolve_Mount(normalized_path); // throws TFile_Not_Found_Exception
			file = mount.Open_File(normalized_path, attributes);
			if (!file) {
				throw TFile_Not_Found_Exception();
			}
			else if ((file->Get_Attributes() == kiv_os::NFile_Attributes::Read_Only) && attributes != kiv_os::NFile_Attributes::Read_Only) {
				throw TPermission_Denied_Exception();
			}
			Cache_File(file);
		}

		Put_File_Descriptor(free_fd, file, attributes);

		return true;
	}

	bool CVirtual_File_System::Create_File(std::string path, kiv_os::NFile_Attributes attributes, kiv_os::THandle &fd_index) {
		fd_index = Get_Free_Fd_Index(); // throws TFd_Table_Full_Exception
		auto normalized_path = Create_Normalized_Path(path); // throws TFile_Not_Found_Exception
		auto mount = Resolve_Mount(normalized_path); // throws TFile_Not_Found_Exception

		// File is cached
		if (Is_File_Cached(normalized_path)) {
			auto file = Get_Cached_File(normalized_path);

			// File is opened -> cannot override this file
			if (file->Is_Opened()) {
				throw TPermission_Denied_Exception();
			}

			// File is not opened -> can override this file (delete it)
			else {
				//file->Get_Mount()->Delete_File(file->Get_Path());
				Decache_File(file);
			}
		}

		auto file = mount.Create_File(normalized_path, attributes);
		if (!file) {
			throw TNot_Enough_Space_Exception();
		}

		Put_File_Descriptor(fd_index, file, attributes);

		return true;
	}

	bool CVirtual_File_System::Close_File(kiv_os::THandle fd_index) {
		auto file_desc = Get_File_Descriptor(fd_index); // Throws TInvalid_Fd_Excep

		Remove_File_Descriptor(fd_index);

		// TODO Remove record in PCB
		return false;
	}

	bool CVirtual_File_System::Delete_File(std::string path) {
		auto normalized_path = Create_Normalized_Path(path); // Throws TFile_Not_Found_Exception

		// File is cached
		if (Is_File_Cached(normalized_path)) {
			auto file = Get_Cached_File(normalized_path);

			// File is opened -> cannot delete that file
			if (file->Is_Opened()) {
				throw TPermission_Denied_Exception();
			}

			// File is not opened -> can delete that file
			else {
				// file->Get_Mount()->Delete_File(file->Get_Path()); fix
				Decache_File(file);
				return true;
			}
		}

		// File is not cached
		else {
			auto mount = Resolve_Mount(normalized_path); // Throws TFile_Not_Found_Exception
			if (mount.Delete_File(normalized_path)) {
				throw TFile_Not_Found_Exception();
			}
		}

		return true;
	}

	size_t CVirtual_File_System::Write_File(kiv_os::THandle fd_index, char *buffer, size_t buffer_size) {
		auto file_desc = Get_File_Descriptor(fd_index); // Throws TInvalid_Fd_Exception

		if (!(file_desc.attributes & FD_ATTR_WRITE)) {
			throw TPermission_Denied_Exception();
		}

		return file_desc.file->Write(buffer, buffer_size, file_desc.position);
	}

	size_t CVirtual_File_System::Read_File(kiv_os::THandle fd_index, char *buffer, size_t buffer_size) {
		auto file_desc = Get_File_Descriptor(fd_index); // throws TInvalid_Fd_Exception

		if (!(file_desc.attributes & FD_ATTR_READ)) {
			throw TPermission_Denied_Exception();
		}

		return file_desc.file->Read(buffer, buffer_size, file_desc.position);
	}

	bool CVirtual_File_System::Set_Position(kiv_os::THandle fd_index, int position, kiv_os::NFile_Seek type) {
		auto file_desc = Get_File_Descriptor(fd_index); // throws TInvalid_Fd_Exception

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

			default:
				throw TInvalid_Operation_Exception();
		}

		tmp_pos += position;

		if (tmp_pos > file_desc.file->Get_Size() || tmp_pos < 0) {
			throw TPosition_Out_Of_Range_Exception();
		}

		file_desc.position = tmp_pos;
		return true;
	}

	bool CVirtual_File_System::Set_Size(kiv_os::THandle fd_index, int position, kiv_os::NFile_Seek type) {
		// TODO
		return false;
	}

	size_t CVirtual_File_System::Get_Position(kiv_os::THandle fd_index) {
		auto file_desc = Get_File_Descriptor(fd_index); // Throws TInvalid_Fd_Exception

		return file_desc.position;
	}

	bool CVirtual_File_System::Create_Pipe(kiv_os::THandle & write_end, kiv_os::THandle & read_end) {
		// TODO
		return false;
	}

	void CVirtual_File_System::Set_Working_Directory(char *path) {
		TPath normalized_path = Create_Normalized_Path(path); // Throws File_Not_Found_Exception
		auto mount = Resolve_Mount(normalized_path); // Throws File_Not_Found_Exception

		if (!mount.Open_File(normalized_path, kiv_os::NFile_Attributes::Read_Only)) { // TODO correct attrs?
			// TODO prevent from deleting working directory?
			throw TFile_Not_Found_Exception();
		}
		
		// TODO process::Set_Working_Directory(normalized_path);
	}

	size_t CVirtual_File_System::Get_Working_Directory(char *buffer, size_t buf_size) {
		TPath tpath; // TODO process::Get_Working_Directory();

		std::string working_dir = tpath.absolute_path;
		if (buf_size < working_dir.length()) {
			return 0;
		}
		
		// UNSAFE std::copy(working_dir.begin(), working_dir.end(), buffer);
		stdext::checked_array_iterator<char *> chai(buffer, buf_size);
		std::copy(working_dir.begin(), working_dir.end(), chai);

		return working_dir.length();
	}

	// ====================
	// ===== PRIVATE ======
	// ====================

	TFile_Descriptor &CVirtual_File_System::Get_File_Descriptor(kiv_os::THandle fd_index) {
		if (!mFile_descriptors[fd_index].file) {
			throw TInvalid_Fd_Exception();
		}

		return mFile_descriptors[fd_index];
	}

	void CVirtual_File_System::Put_File_Descriptor(kiv_os::THandle fd_index, std::shared_ptr<IFile> file, kiv_os::NFile_Attributes attributes) {
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

	void CVirtual_File_System::Remove_File_Descriptor(kiv_os::THandle fd_index) {
		TFile_Descriptor &file_desc = mFile_descriptors[fd_index];

		mFd_count--;
		Decrease_File_References(file_desc);

		file_desc.file = nullptr;
		file_desc.attributes = FD_ATTR_FREE;
	}

	kiv_os::THandle CVirtual_File_System::Get_Free_Fd_Index() {
		if (mFd_count == MAX_FILE_DESCRIPTORS) {
			throw TFd_Table_Full_Exception();
		}

		// TODO optimize (keep last free fd that was found?)
		for (kiv_os::THandle i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
			if (mFile_descriptors[i].file == nullptr) { // TODO (compare attribute with FD_ATTR_FREE)
				return i;
			}
		}

		return -1; // fix hack not all paths lead to return --- 
	}

	IMounted_File_System &CVirtual_File_System::Resolve_Mount(const TPath &normalized_path) {
		// TODO throw TFile_Not_Found_Exception when bad mount
		return *mMounted_file_systems.at(normalized_path.mount);
	}

	bool CVirtual_File_System::Is_File_Cached(const TPath& path) {
		return (mCached_files.find(path.absolute_path) != mCached_files.end());
	}

	void CVirtual_File_System::Cache_File(std::shared_ptr<IFile> &file) {
		mCached_files.insert(std::pair<std::string, std::shared_ptr<IFile>>(file->Get_Path().absolute_path, file));
	}

	void CVirtual_File_System::Decache_File(std::shared_ptr<IFile> &file) {
		if (!Is_File_Cached(file->Get_Path())) {
			throw TInternal_Error_Exception();
		}

		mCached_files.erase(mCached_files.find(file->Get_Path().absolute_path));
	}

	std::shared_ptr<IFile> CVirtual_File_System::Get_Cached_File(const TPath &path) {
		return mCached_files.find(path.absolute_path)->second;
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
		// Known issues 
		//	- two ".." on the end are handled incorrectly (e.g. "a\\b\\..\\..")
		//	- process's working directory will be TPath, not string
		//	- absolute path is not yet filled

		// Move these constants
		std::string path_delimiter = "\\";
		std::string mount_delimiter = ":";
		size_t mount_max_size = 6;

		TPath result;

		std::vector<std::string> splitted_by_mount = Split(path, mount_delimiter);

		// Absolute path
		if (splitted_by_mount.size() == 2) {
			result.mount = splitted_by_mount.at(0);
			result.path = Split(splitted_by_mount.at(1), path_delimiter);
		}

		// Relative path
		else if (splitted_by_mount.size() == 1) {
			std::string working_dir = "C:\\aa\\bb\\cc"; // TODO process:Get_Working_Dir();

			std::vector<std::string> working_dir_splitted_by_mount = Split(working_dir, mount_delimiter);

			result.mount = working_dir_splitted_by_mount.at(0);

			auto wokring_splitted = Split(working_dir_splitted_by_mount.at(1), path_delimiter);
			auto path_splitted = Split(splitted_by_mount.at(0), path_delimiter);

			// Concatenate working directory and path
			result.path.reserve(wokring_splitted.size() + path_splitted.size());
			result.path.insert(result.path.end(), wokring_splitted.begin(), wokring_splitted.end());
			result.path.insert(result.path.end(), path_splitted.begin(), path_splitted.end());
		}

		// Wrong format (multiple mount separators)
		else {
			throw TFile_Not_Found_Exception();
		}

		// Handle dots
		for (auto itr = result.path.begin(); itr != result.path.end(); ++itr) {
			if (*itr == "..") {
				// ".." on the root -> do nothing
				if (itr == result.path.begin()) {
					itr = result.path.erase(itr);
				}

				// ".." is at the end
				else if ((itr + 1) == result.path.end()) {
					result.path.pop_back();
					if (!result.path.empty())
						result.path.pop_back();
					break;
				}

				else {
					itr = result.path.erase(itr - 1, itr + 1);
				}
			}

			else if (*itr == ".") {
				itr = result.path.erase(itr);
			}
		}

		// Remove filename from 'result->path' and insert it into a 'result->file'
		result.file = result.path.at(result.path.size() - 1);
		result.path.erase(result.path.end() - 1);

		return result;
	}

#pragma endregion

}
