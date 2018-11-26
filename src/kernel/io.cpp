#include "io.h"
#include "kernel.h"
#include "handles.h"
#include "vfs.h"
#include "process.h"

kiv_vfs::CVirtual_File_System &vfs = kiv_vfs::CVirtual_File_System::Get_Instance();

void Set_Result(kiv_hal::TRegisters &regs, kiv_os::NOS_Error result) {
	regs.flags.carry = (result == kiv_os::NOS_Error::Success) ? 0 : 1;
	if (regs.flags.carry) {
		regs.rax.x = static_cast<decltype(regs.rax.x)>(result);
	}
}

kiv_os::NOS_Error Get_Position(kiv_os::THandle vfs_handle, kiv_hal::TRegisters &regs) {
	try {
		size_t position = vfs.Get_Position(vfs_handle);
		regs.rax.r = static_cast<decltype(regs.rax.r)>(position);
		return kiv_os::NOS_Error::Success;
	}
	catch (...) { // Including TInvalid_Fd_Exception
		return kiv_os::NOS_Error::Unknown_Error;
	}
}

kiv_os::NOS_Error Set_Position(kiv_os::THandle vfs_handle, int position, kiv_os::NFile_Seek seek_offset_type) {
	try {
		vfs.Set_Position(vfs_handle, position, seek_offset_type);
		return kiv_os::NOS_Error::Success;
	}
	catch (kiv_vfs::TPosition_Out_Of_Range_Exception e) {
		return kiv_os::NOS_Error::Invalid_Argument; // TODO Correct error?
	}
	catch (...) { // Including TInvalid_Fd_Exception
		return kiv_os::NOS_Error::Unknown_Error;
	}
}

kiv_os::NOS_Error Set_Size(kiv_os::THandle vfs_handle, int position, kiv_os::NFile_Seek seek_offset_type) {
	try {
		vfs.Set_Size(vfs_handle, position, seek_offset_type);
		return kiv_os::NOS_Error::Success;
	}
	catch (kiv_vfs::TNot_Enough_Space_Exception e) {
		return kiv_os::NOS_Error::Not_Enough_Disk_Space;
	}
	catch (...) {
		return kiv_os::NOS_Error::Unknown_Error;
	}
}

void Seek(kiv_hal::TRegisters &regs) {
	kiv_os::THandle proc_handle = static_cast<kiv_os::THandle>(regs.rdx.x);
	kiv_os::NFile_Seek seek_type = static_cast<kiv_os::NFile_Seek>(regs.rcx.l);
	kiv_os::NFile_Seek seek_offset_type = static_cast<kiv_os::NFile_Seek>(regs.rcx.h);
	int position = static_cast<int>(regs.rdi.r);

	kiv_os::THandle vfs_handle;
	if (!kiv_process::CProcess_Manager::Get_Instance().Get_Fd(proc_handle, vfs_handle)) {
		Set_Result(regs, kiv_os::NOS_Error::Unknown_Error);
		return;
	}

	kiv_os::NOS_Error result;
	switch (seek_type) {
		case kiv_os::NFile_Seek::Get_Position:
			result = Get_Position(vfs_handle, regs);
			break;

		case kiv_os::NFile_Seek::Set_Position:
			result = Set_Position(vfs_handle, position, seek_offset_type);
			break;

		case kiv_os::NFile_Seek::Set_Size:
			result = Set_Size(vfs_handle, position, seek_offset_type);
			break;
	}

	Set_Result(regs, result);
}

void Open_File(kiv_hal::TRegisters &regs) {
	std::string path = reinterpret_cast<char *>(regs.rdx.r);
	kiv_os::NFile_Attributes attributes = static_cast<kiv_os::NFile_Attributes>(regs.rdi.r);
	kiv_os::NOpen_File open_flag = static_cast<kiv_os::NOpen_File>(regs.rcx.r);

	kiv_os::NOS_Error result;
	try {
		kiv_os::THandle handle;
		if (open_flag == kiv_os::NOpen_File::fmOpen_Always) {
			vfs.Open_File(path, attributes, handle);
		}
		else {
			vfs.Create_File(path, attributes, handle);
		}
		handle = kiv_process::CProcess_Manager::Get_Instance().Save_Fd(handle);

		regs.rax.x = static_cast<decltype(regs.rax.x)>(handle);
		result = kiv_os::NOS_Error::Success;
	}
	catch (kiv_vfs::TFd_Table_Full_Exception e) {
		result = kiv_os::NOS_Error::Out_Of_Memory;
	}
	catch (kiv_vfs::TPermission_Denied_Exception e) {
		result = kiv_os::NOS_Error::Permission_Denied;
	}
	catch (kiv_vfs::TFile_Not_Found_Exception e) {
		result = kiv_os::NOS_Error::File_Not_Found;
	}
	catch (kiv_vfs::TNot_Enough_Space_Exception e) { // In case of creating new file
		result = kiv_os::NOS_Error::Not_Enough_Disk_Space;
	}
	catch (...) {
		result = kiv_os::NOS_Error::Unknown_Error;
	}

	Set_Result(regs, result);
}

void Close_File(kiv_hal::TRegisters &regs) {
	kiv_os::THandle proc_handle = static_cast<kiv_os::THandle>(regs.rdx.x);

	kiv_os::THandle vfs_handle;
	if (!kiv_process::CProcess_Manager::Get_Instance().Get_Fd(proc_handle, vfs_handle)) {
		Set_Result(regs, kiv_os::NOS_Error::File_Not_Found);
		return;
	}

	kiv_os::NOS_Error result;
	try {
		vfs.Close_File(vfs_handle);
		kiv_process::CProcess_Manager::Get_Instance().Remove_Fd(proc_handle);
		result = kiv_os::NOS_Error::Success;
	}
	catch (...) { // including TInvalid_Fd_Exception
		result = kiv_os::NOS_Error::Unknown_Error;
	}

	Set_Result(regs, result);
}

void Delete_File(kiv_hal::TRegisters &regs) {
	std::string path = reinterpret_cast<char *>(regs.rdx.r);

	kiv_os::NOS_Error result;
	try {
		vfs.Delete_File(path);
		result = kiv_os::NOS_Error::Success;
	}
	catch (kiv_vfs::TFile_Not_Found_Exception e) {
		result = kiv_os::NOS_Error::File_Not_Found;
	}
	catch (kiv_vfs::TDirectory_Not_Empty_Exception e) {
		result = kiv_os::NOS_Error::Directory_Not_Empty;
	}
	catch (kiv_vfs::TPermission_Denied_Exception e) {
		result = kiv_os::NOS_Error::Permission_Denied;
	}
	catch (...) {
		result = kiv_os::NOS_Error::Unknown_Error;
	}

	Set_Result(regs, result);
}

void Write_File(kiv_hal::TRegisters &regs) {
	kiv_os::THandle proc_handle = static_cast<kiv_os::THandle>(regs.rdx.x);
	char *buffer = reinterpret_cast<char *>(regs.rdi.r);
	size_t buf_size = static_cast<size_t>(regs.rcx.r);

	size_t bytes_written = 0;

	kiv_os::THandle vfs_handle;
	if (!kiv_process::CProcess_Manager::Get_Instance().Get_Fd(proc_handle, vfs_handle)) {
		Set_Result(regs, kiv_os::NOS_Error::File_Not_Found);
		return;
	}

	kiv_os::NOS_Error result;
	try {
		bytes_written = vfs.Write_File(vfs_handle, buffer, buf_size);
		result = kiv_os::NOS_Error::Success;
	}
	catch (kiv_vfs::TPermission_Denied_Exception e) {
		result = kiv_os::NOS_Error::Permission_Denied;
	}
	catch (...) { // including TInvalid_Fd_Exception
		result = kiv_os::NOS_Error::Unknown_Error;
	}

	regs.rax.r = bytes_written;
	Set_Result(regs, result);
}

void Read_File(kiv_hal::TRegisters &regs) {
	kiv_os::THandle proc_handle = static_cast<kiv_os::THandle>(regs.rdx.x);
	char *buffer = reinterpret_cast<char *>(regs.rdi.r);
	size_t buf_size = static_cast<size_t>(regs.rcx.r);

	size_t bytes_read = 0;

	kiv_os::THandle vfs_handle;
	if (!kiv_process::CProcess_Manager::Get_Instance().Get_Fd(proc_handle, vfs_handle)) {
		Set_Result(regs, kiv_os::NOS_Error::File_Not_Found);
		return;
	}

	kiv_os::NOS_Error result;
	try {
		bytes_read = vfs.Read_File(vfs_handle, buffer, buf_size);
		result = kiv_os::NOS_Error::Success;
	}
	catch (kiv_vfs::TPermission_Denied_Exception e) {
		result = kiv_os::NOS_Error::Permission_Denied;
	}
	catch (...)	{ 
		result = kiv_os::NOS_Error::Unknown_Error; 
	}

	regs.rax.r = bytes_read;
	Set_Result(regs, result);
}

void Set_Working_Dir(kiv_hal::TRegisters &regs) {
	char *path = reinterpret_cast<char *>(regs.rdx.r); // null terminated

	kiv_os::NOS_Error result;
	try {
		vfs.Set_Working_Directory(path);
		result = kiv_os::NOS_Error::Success;
	}
	catch (kiv_vfs::TFile_Not_Found_Exception e) {
		result = kiv_os::NOS_Error::File_Not_Found;
	}
	catch (...) {
		result = kiv_os::NOS_Error::Unknown_Error;
	}

	Set_Result(regs, result);
}

void Get_Working_Dir(kiv_hal::TRegisters &regs) {
	char *buffer = reinterpret_cast<char *>(regs.rdx.r);
	size_t buf_size = static_cast<size_t>(regs.rcx.r);

	size_t chars_written = 0;
	kiv_os::NOS_Error result = kiv_os::NOS_Error::Unknown_Error;

	kiv_vfs::TPath working_dir;
	if (kiv_process::CProcess_Manager::Get_Instance().Get_Working_Directory(&working_dir)) {
		size_t wd_length = working_dir.absolute_path.length();
		// Check whether buffer is big enough
		if (buf_size >= wd_length) { 
			memcpy(buffer, working_dir.absolute_path.c_str(), wd_length);
			chars_written = wd_length;
			result = kiv_os::NOS_Error::Success;
		}
	}

	regs.rax.r = static_cast<decltype(regs.rax.r)>(chars_written);
	Set_Result(regs, result);
}

void Create_Pipe(kiv_hal::TRegisters &regs) {
	kiv_os::NOS_Error result;
	kiv_os::THandle *handle_pair = reinterpret_cast<kiv_os::THandle *>(regs.rdx.r);
	kiv_os::THandle in;
	kiv_os::THandle out;

	try {
		vfs.Create_Pipe(out, in);
		result = kiv_os::NOS_Error::Success;
	}
	catch (kiv_vfs::TFd_Table_Full_Exception e) {
		result = kiv_os::NOS_Error::Out_Of_Memory;
	}
	catch (...) {
		result = kiv_os::NOS_Error::Unknown_Error;
	}

	handle_pair[0] = out;
	handle_pair[1] = in;
	
	Set_Result(regs, result);
}


void Handle_IO(kiv_hal::TRegisters &regs) {
	switch (static_cast<kiv_os::NOS_File_System>(regs.rax.l)) {
		case kiv_os::NOS_File_System::Open_File:		
			Open_File(regs);
			break;
		case kiv_os::NOS_File_System::Close_Handle:	
			Close_File(regs);
			break;
		case kiv_os::NOS_File_System::Delete_File: 
			Delete_File(regs);
			break;
		case kiv_os::NOS_File_System::Write_File: 
			Write_File(regs);
			break;
		case kiv_os::NOS_File_System::Read_File: 
			Read_File(regs);
			break;
		case kiv_os::NOS_File_System::Seek: 
			Seek(regs);
			break;
		case kiv_os::NOS_File_System::Set_Working_Dir: 
			Set_Working_Dir(regs);
			break;
		case kiv_os::NOS_File_System::Get_Working_Dir:
			Get_Working_Dir(regs);
			break;
		case kiv_os::NOS_File_System::Create_Pipe: 
			Create_Pipe(regs);
			break;
		default:
			Set_Result(regs, kiv_os::NOS_Error::Unknown_Error);
			break;
	}
}