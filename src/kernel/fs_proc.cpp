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
	CFile::CFile(const kiv_vfs::TPath path, size_t pid, std::string name) {
		mPath = path;
		mAttributes = kiv_os::NFile_Attributes::Read_Only;
		mName = name;
		if (name.length() < 7) { // hmm
			mName +=  "\t\t" + std::to_string(pid);
		}
		else {
			mName += "\t" + std::to_string(pid);
		}
	}

	kiv_os::NOS_Error CFile::Read(char *buffer, size_t buffer_size, size_t position, size_t &read) {
		if (position > mName.length()) {
			read = 0;
			return kiv_os::NOS_Error::Invalid_Argument;
		}

		std::string str_to_write = mName.substr(position, position + buffer_size - 1);
		strcpy_s(buffer, buffer_size, str_to_write.c_str());
		read = str_to_write.length();
		return kiv_os::NOS_Error::Success;
	}

	bool CFile::Is_Available_For_Write() {
		return false;
	}
#pragma endregion

#pragma region Directory
	CDirectory::CDirectory(const kiv_vfs::TPath path, const std::map<size_t, std::string> &processes) : mProcesses(processes) {
		mPath = path;
		mAttributes = kiv_os::NFile_Attributes::Directory;
	}

	CDirectory::CDirectory(const kiv_vfs::TPath path)  {
		mPath = path;
	}

	kiv_os::NOS_Error CDirectory::Read(char *buffer, size_t buffer_size, size_t position, size_t &read) {
		size_t i = 0;
		kiv_os::TDir_Entry entry = {};
		for (auto it = mProcesses.begin(); it != mProcesses.end(); ++it) {
			if (i == position) {
				strcpy_s(entry.file_name, file_name_size, std::to_string(it->first).c_str());
				entry.file_attributes = static_cast<uint16_t>(kiv_os::NFile_Attributes::Read_Only);
				break;
			}
			i++;
		}

		if (i == mProcesses.size()) {
			read = 0;
			return kiv_os::NOS_Error::Success;
		}

		if (sizeof(entry) > buffer_size) {
			read = 0;
			return kiv_os::NOS_Error::Invalid_Argument;
		}

		memcpy(buffer, &entry, sizeof(entry));
		read = 1;
		return kiv_os::NOS_Error::Success;
	}

	bool CDirectory::Is_Available_For_Write() {
		return false;
	}

	void CDirectory::Refresh_Processes(const std::map<size_t, std::string> &processes) {
		mProcesses = processes;
	}
#pragma endregion

#pragma region Mount
	CMount::CMount(std::string label) { 
		mLabel = label;
		kiv_vfs::TPath path;
		path.mount = label;
		path.file = "";

		mRoot = std::make_shared<CDirectory>(path);
	}

	kiv_os::NOS_Error CMount::Open_File(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes, std::shared_ptr<kiv_vfs::IFile> &file) {
		std::string name;

		if (path.mount == mRoot->Get_Path().mount && path.file.length() == 0) {
			mRoot = std::make_shared<CDirectory>(path, kiv_process::CProcess_Manager::Get_Instance().Get_Processes());
			file = mRoot;
			return kiv_os::NOS_Error::Success;
		}

		std::istringstream iss(path.file);
		size_t pid;
		iss >> pid;

		if (iss.fail()) {
			return kiv_os::NOS_Error::Unknown_Error; // TODO Spravnej error?
		}
		if (kiv_process::CProcess_Manager::Get_Instance().Get_Name(pid, name)) {
			file = std::make_shared<CFile>(path, pid, name);
			return kiv_os::NOS_Error::Success;
		}
		else {
			return kiv_os::NOS_Error::File_Not_Found;
		}
	}
#pragma endregion
}
