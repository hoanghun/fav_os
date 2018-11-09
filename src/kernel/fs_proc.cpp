#include "fs_proc.h"

namespace kiv_fs_proc {

#pragma region File system

	CFile_System::CFile_System() {
		mName = "fs_proc";
	}

	bool CFile_System::Register() {
		return kiv_vfs::CVirtual_File_System::Get_Instance().Register_File_System(*this);
	}

	kiv_vfs::IMounted_File_System *CFile_System::Create_Mount(const std::string label, const kiv_vfs::TDisk_Number disk_number) {
		return new CMount(label, disk_number);
	}

#pragma endregion


#pragma region File

	CFile::CFile(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes) {
		mPath = path;
		mAttributes = attributes;
	}

	size_t CFile::Write(const char *buffer, size_t buffer_size, int position) {
		// TODO implement
		return 0;
	}

	size_t CFile::Read(char *buffer, size_t buffer_size, int position) {
		// TODO implement
		return 0;
	}

	bool CFile::Is_Available_For_Write() {
		return false;
	}

#pragma endregion


#pragma region Mount

	CMount::CMount(std::string label, kiv_vfs::TDisk_Number disk) { 
		mLabel = label;
		mDisk = disk;
	}

	std::shared_ptr<kiv_vfs::IFile> CMount::Open_File(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes) {
		// TODO implement
		return nullptr;
	}

	std::shared_ptr<kiv_vfs::IFile> CMount::Create_File(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes) {
		// TODO implement
		return nullptr;
	}

	bool CMount::Delete_File(const kiv_vfs::TPath &path) {
		// TODO implement
		return false;
	}

	kiv_vfs::TDisk_Number CMount::Get_Disk_Number() {
		return mDisk;
	}

#pragma endregion

}
