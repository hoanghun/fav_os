#include "vfs.h"

namespace kiv_vfs {

#pragma region File

	void IFile::Increase_Reference_Count() {
		mReference_count++;
	}

	void IFile::Decrease_Reference_Count() {
		mReference_count--;
	}

	std::string IFile::Get_Name() {
		return mName;
	}

	IMounted_File_System *IFile::Get_Mount() {
		return mMount;
	}

	unsigned int IFile::Get_Reference_Count() {
		return mReference_count;
	}

	kiv_os::NFile_Attributes IFile::Get_Attributes() {
		return mAttributes;
	}

	bool IFile::Is_Available_For_Writting() {
		return mAvailable_For_Writting;
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

	// Default methods (concrete filesystem can override those methods)
	IFile *IMounted_File_System::Open_File() {
		return nullptr;
	}

	bool IMounted_File_System::Delete_File() {
		return false;
	}
#pragma endregion


#pragma region Virtual file system

	CVirtual_File_System *CVirtual_File_System::instance = new CVirtual_File_System();

	CVirtual_File_System::CVirtual_File_System() {
		// TODO 
	}

	void CVirtual_File_System::Destroy() {
		// TODO cleanup
		delete instance;
	}

	CVirtual_File_System &CVirtual_File_System::Get_Instance() {
		return *instance;
	}

	bool CVirtual_File_System::Register_File_System(IFile_System &fs) {
		return false;
	}

	bool CVirtual_File_System::Mount_File_System(std::string fs_name, std::string label, TDisk_Number disk) {
		return false;
	}

#pragma endregion

}
