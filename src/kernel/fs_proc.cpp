#include "fs_proc.h"
#include "process.h"

#include <sstream>
const size_t file_name_size = 12;

namespace kiv_fs_proc {

#pragma region File system

	CFile_System::CFile_System() {
		mName = "fs_proc";
	}

	kiv_vfs::IMounted_File_System *CFile_System::Create_Mount(const std::string label, const kiv_vfs::TDisk_Number disk_number) {
		return new CMount(label);
	}

#pragma endregion


#pragma region File
	CFile::CFile(const kiv_vfs::TPath path, size_t pid) : mPid(pid) {
		mPath = path;
		mAttributes = kiv_os::NFile_Attributes::Read_Only;
	}

	size_t CFile::Read(char *buffer, size_t buffer_size, size_t position) {
		strcpy_s(buffer, buffer_size, mName.substr(position, buffer_size - 1).c_str());
		return 0;
	}

	bool CFile::Is_Available_For_Write() {
		return false;
	}

#pragma endregion

#pragma region Directory
	CDirectory::CDirectory(const kiv_vfs::TPath path, const std::map<size_t, std::string> &processes) : mProcesses(processes) {
		mPath = path;
	}

	size_t CDirectory::Read(char *buffer, size_t buffer_size, size_t position) {
		size_t i = 0;
		kiv_os::TDir_Entry entry;
		for (auto it = mProcesses.begin(); it != mProcesses.end(); ++it) {
			if (i == position) {
				strcpy_s(entry.file_name, file_name_size, std::to_string(it->first).c_str());
				entry.file_attributes = static_cast<uint16_t>(kiv_os::NFile_Attributes::Read_Only);
				break;
			}
			i++;
		}

		if (i == mProcesses.size()) {
			return 0;
		}

		if (sizeof(entry) > buffer_size) {
			throw new kiv_vfs::TInvalid_Operation_Exception();
		}

		memcpy(buffer, &entry, sizeof(entry));
		return 1;
	}

	bool CDirectory::Is_Available_For_Write() {
		return false;
	}

#pragma endregion

#pragma region Mount

	CMount::CMount(std::string label) { 
		mLabel = label;
		kiv_vfs::TPath path;
		path.absolute_path = label;

		mRoot = std::make_shared<CDirectory>(path, kiv_process::CProcess_Manager::Get_Instance().Get_Processes());
	}

	std::shared_ptr<kiv_vfs::IFile> CMount::Open_File(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes) {
		std::string name;

		if (path.absolute_path == mRoot->Get_Path().absolute_path) {
			mRoot = std::make_shared<CDirectory>(path, kiv_process::CProcess_Manager::Get_Instance().Get_Processes());
			return mRoot;
		}

		std::istringstream iss(path.file);
		size_t pid;
		iss >> pid;

		if (iss.fail()) {
			return nullptr;
		}

		if (kiv_process::CProcess_Manager::Get_Instance().Get_Name(pid, name)) {
			std::shared_ptr<kiv_vfs::IFile> file = std::make_shared<CFile>(path, pid);
		}
			
		return nullptr;
	}

#pragma endregion
}
