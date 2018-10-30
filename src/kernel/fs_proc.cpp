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

	CFile::CFile(std::string name, CMount *mount, kiv_os::NFile_Attributes attributes) {
		mName = name;
		mMount = mount;
		mAttributes = attributes;
		mAvailable_For_Writting = true;
	}

	unsigned int CFile::Write(int position, char *buffer, size_t buffer_size) {
		// TODO implement
		return 0;
	}

	unsigned int CFile::Read(int position, char *buffer, size_t buffer_size) {
		// TODO implement
		return 0;
	}

#pragma endregion


#pragma region Mount

	CMount::CMount(std::string label, kiv_vfs::TDisk_Number disk) { 
		mLabel = label;
		mDisk = disk;
	}

	kiv_vfs::IFile *CMount::Open_File(std::string path, kiv_os::NFile_Attributes attributes, bool create_new) {
		// TODO implement
		return nullptr;
	}

	bool CMount::Delete_File(std::string path) {
		// TODO implement
		return false;
	}

	kiv_vfs::TDisk_Number CMount::Get_Disk_Number() {
		return mDisk;
	}

#pragma endregion

}
