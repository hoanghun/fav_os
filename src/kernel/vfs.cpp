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

#pragma endregion


#pragma region File system

	std::string IFile_System::Get_Name() {
		return mName;
	}

#pragma endregion


#pragma region Mounted file system
	std::string IMounted_File_System::Get_Label() {
		return mLabel;
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
			auto mount = reg_file_system->Create_Mount("?");
			mMounted_file_systems.insert(std::make_pair("?", mount)); // TODO 
		}
	}

	void CVirtual_File_System::UMount() {
		for (auto mount : mMounted_file_systems) {
			delete mount.second;
		}
	}

	bool CVirtual_File_System::Open_File(std::string path, kiv_os::NFile_Attributes attributes, kiv_os::THandle &fd_index) {
		kiv_os::THandle free_fd = Get_Free_Fd_Index(); // throws TFd_Table_Full_Exception
		auto normalized_path = Create_Normalized_Path(path); // throws TInvalid_Path_Exception

		std::shared_ptr<IFile> file;

		// File is cached
		if (Is_File_Cached(normalized_path)) {
			file = Get_Cached_File(normalized_path);
		}

		// File is not cached -> resolve file and cache file
		else {
			auto mount = Resolve_Mount(normalized_path); // throws TInvalid_Mount_Exception
			file = mount.Open_File(normalized_path, attributes);
			if (!file) {
				throw TFile_Not_Found_Exception();
			}
			Cache_File(file);
		}

		auto file_desc = Create_File_Descriptor(file, attributes);
		Put_File_Descriptor(free_fd, file_desc);

		return true;
	}

	bool CVirtual_File_System::Create_File(std::string path, kiv_os::NFile_Attributes attributes, kiv_os::THandle &fd_index) {
		// TODO
		// Zkontrolovat jestli neni v cache (smazat jestli jo)

		return true;
	}

	bool CVirtual_File_System::Close_File(kiv_os::THandle fd_index) {
		auto file_desc = Get_File_Descriptor(fd_index);

		Decrease_File_References(file_desc);
		// Remove file descriptor
		mFile_descriptors[fd_index].file = nullptr;

		// TODO Remove record in PCB
		return false;
	}

	bool CVirtual_File_System::Delete_File(std::string path) {
		auto normalized_path = Create_Normalized_Path(path); // Throws TInvalid_Path_Exception

		// File is cached
		if (Is_File_Cached(normalized_path)) {
			auto it = mCached_files.find(path);
			auto file = it->second;

			// File is opened -> cannot delete that file
			if (file->Is_Opened()) {
				return false;
			}

			// File is not opened -> can delete that file
			else {
				// file->Get_Mount()->Delete_File(file->Get_Path()); fix
				mCached_files.erase(it);
				return true;
			}
		}

		// File is not cached
		else {
			auto mount = Resolve_Mount(normalized_path);
			mount.Delete_File(normalized_path);
		}
		return true;
	}

	size_t CVirtual_File_System::Write_File(kiv_os::THandle fd_index, char *buffer, size_t buffer_size) {
		auto file_desc = Get_File_Descriptor(fd_index);

		if (!file_desc.file) {
			throw new kiv_vfs::TFile_Not_Found_Exception();
		}

		if (!(file_desc.attributes & kiv_vfs::FD_ATTR_WRITE)) {
			throw new kiv_vfs::TRead_Only();
		}

		return file_desc.file->Write(buffer, buffer_size, file_desc.position);
	}

	size_t CVirtual_File_System::Read_File(kiv_os::THandle fd_index, char *buffer, size_t buffer_size) {
		auto file_desc = Get_File_Descriptor(fd_index);

		// TODO
		return 0;
	}

	bool CVirtual_File_System::Set_Position(kiv_os::THandle fd_index, int position, kiv_os::NFile_Seek type) {
		auto file_desc = Get_File_Descriptor(fd_index);

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
				return false;
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
		auto file_desc = Get_File_Descriptor(fd_index); // T

		return file_desc.position;
	}

	bool CVirtual_File_System::Create_Pipe(kiv_os::THandle & write_end, kiv_os::THandle & read_end) {
		// TODO
		return false;
	}

	// ====================
	// ===== PRIVATE ======
	// ====================

	TFile_Descriptor CVirtual_File_System::Create_File_Descriptor(std::shared_ptr<IFile> file, kiv_os::NFile_Attributes attributes) {
		TFile_Descriptor file_desc;

		file_desc.position = 0;
		file_desc.file = file;
		if (attributes == kiv_os::NFile_Attributes::Read_Only) {
			file_desc.attributes = FD_ATTR_READ;
		}
		else {
			file_desc.attributes = FD_ATTR_RW;
		}

		Increase_File_References(file, attributes);

		return file_desc;
	}

	TFile_Descriptor &CVirtual_File_System::Get_File_Descriptor(kiv_os::THandle fd_index) {
		if (!mFile_descriptors[fd_index].file) {
			throw TInvalid_Fd_Exception();
		}

		return mFile_descriptors[fd_index];
	}

	void CVirtual_File_System::Put_File_Descriptor(kiv_os::THandle fd_index, TFile_Descriptor &file_desc) {
		mFd_count++;
		mFile_descriptors[fd_index] = file_desc;
	}

	kiv_os::THandle CVirtual_File_System::Get_Free_Fd_Index() {
		if (mFd_count == MAX_FILE_DESCRIPTORS) {
			throw TFd_Table_Full_Exception();
		}

		// TODO optimize (keep last free fd that was found?)
		for (kiv_os::THandle i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
			if (mFile_descriptors[i].file == nullptr) {
				return i;
			}
		}

		return -1; // fix hack not all paths lead to return --- 
	}

	IMounted_File_System &CVirtual_File_System::Resolve_Mount(const TPath &normalized_path) {
		return *mMounted_file_systems.at(normalized_path.mount);
	}

	TPath CVirtual_File_System::Create_Normalized_Path(std::string path) {
		// TODO
		// TODO get process's working directory
		throw TInvalid_Path_Exception(); 
		return TPath();
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

	void CVirtual_File_System::Increase_File_References(std::shared_ptr<IFile> &file, kiv_os::NFile_Attributes attributes) {
		if (attributes == kiv_os::NFile_Attributes::Read_Only) {
			file->Increase_Read_Count();
		}
		else {
			file->Increase_Read_Count();
			file->Increase_Write_Count();
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

#pragma endregion

}
