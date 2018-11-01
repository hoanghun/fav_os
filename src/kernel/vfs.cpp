#include "vfs.h"

namespace kiv_vfs {

#pragma region File

	// Default implementation (concrete filesystem can override this method)
	unsigned int IFile::Get_Size() {
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

	std::string IFile::Get_Path() {
		return mPath;
	}

	IMounted_File_System *IFile::Get_Mount() {
		return mMount;
	}

	unsigned int IFile::Get_Write_Count() {
		return mWrite_count;
	}

	unsigned int IFile::Get_Read_Count() {
		return mRead_count;
	}

	unsigned int IFile::Get_Reference_Count() {
		return Get_Write_Count() + Get_Read_Count();
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

	// Default implementation (concrete filesystem can override those methods)
	IFile *IMounted_File_System::Open_File() {
		return nullptr;
	}

	bool IMounted_File_System::Delete_File(IFile *file) {
		return false;
	}
#pragma endregion


#pragma region Virtual file system

	CVirtual_File_System *CVirtual_File_System::instance = new CVirtual_File_System();

	CVirtual_File_System::CVirtual_File_System() {
		// TODO 
	}

	void CVirtual_File_System::Destroy() {
		// TODO cleanup?
		delete instance;
	}

	CVirtual_File_System &CVirtual_File_System::Get_Instance() {
		return *instance;
	}

	bool CVirtual_File_System::Register_File_System(IFile_System &fs) {
		// TODO
		return false;
	}

	bool CVirtual_File_System::Mount_File_System(std::string fs_name, std::string label, TDisk_Number disk) {
		// TODO
		return false;
	}

	bool CVirtual_File_System::Open_File(std::string path, kiv_os::NFile_Attributes attributes, bool create_new) {
		// TODO
		return false;
	}

	bool CVirtual_File_System::Close_File(kiv_os::THandle fd) {
		TFile_Descriptor *file_desc = mFile_descriptors[fd];
		if (!file_desc) {
			return false;
		}

		// Decrease counters
		if (file_desc->attributes & FD_ATTR_READ) {
			mFile_descriptors[fd]->file->Decrease_Read_Count();
		}
		if (file_desc->attributes & FD_ATTR_WRITE) {
			mFile_descriptors[fd]->file->Decrease_Write_Count();
		}

		// Remove file descriptor
		mFile_descriptors[fd] = nullptr;
		
		// TODO Remove record in PCB
		return false;
	}

	bool CVirtual_File_System::Delete_File(std::string path) {
		std::map<std::string, IFile *>::iterator it = mCached_files.find(path);

		// File is in the table of opened files
		if (it != mCached_files.end()) {
			IFile *file = it->second;

			// File is opened -> cannot delete that file
			if (file->Get_Reference_Count() != 0) {
				return false;
			}
			else {
				file->Get_Mount()->Delete_File(file->);
				mCached_files.erase(it);
				return true;
			}
		}

		// TODO 
		// fs = Resolve filesystem mount from path
		// fs->delete_file(path);
		return true;
	}

	unsigned int CVirtual_File_System::Write_File(kiv_os::THandle fd, char *buffer, size_t buffer_size) {
		// TODO
		return 0;
	}

	unsigned int CVirtual_File_System::Read_File(kiv_os::THandle fd, char *buffer, size_t buffer_size) {
		// TODO
		return 0;
	}

	bool CVirtual_File_System::Set_Position(kiv_os::THandle fd, int position, kiv_os::NFile_Seek type) {
		TFile_Descriptor *file_desc = mFile_descriptors[fd];
		if (!file_desc) {
			return false;
		}

		unsigned int tmp_pos;
		switch (type) {
			case kiv_os::NFile_Seek::Beginning:
				tmp_pos = 0;
				break;

			case kiv_os::NFile_Seek::Current:
				tmp_pos = file_desc->position;
				break;

			case kiv_os::NFile_Seek::End:
				tmp_pos = file_desc->file->Get_Size() - 1;
				break;

			default:
				return false;
		}

		tmp_pos += position;

		if (tmp_pos > file_desc->file->Get_Size() || tmp_pos < 0) {
			return false;
		}

		file_desc->position = tmp_pos;
		return true;
	}

	bool CVirtual_File_System::Set_Size(kiv_os::THandle fd, int position, kiv_os::NFile_Seek type) {
		// TODO
		return false;
	}

	unsigned int CVirtual_File_System::Get_Position(kiv_os::THandle fd) {
		TFile_Descriptor *file_desc = mFile_descriptors[fd];
		if (!file_desc) {
			// Return error (throw exception?)
			return -1;
		}

		return file_desc->position;
	}

	bool CVirtual_File_System::Create_Pipe(kiv_os::THandle & write_end, kiv_os::THandle & read_end) {
		// TODO
		return false;
	}

#pragma endregion

}
