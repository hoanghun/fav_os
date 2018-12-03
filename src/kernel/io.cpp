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
	size_t position;

	kiv_os::NOS_Error result = vfs.Get_Position(vfs_handle, position);
	regs.rax.r = static_cast<decltype(regs.rax.r)>(position);

	return result;
}

kiv_os::NOS_Error Set_Position(kiv_os::THandle vfs_handle, int position, kiv_os::NFile_Seek seek_offset_type) {
	return vfs.Set_Position(vfs_handle, position, seek_offset_type);
}

kiv_os::NOS_Error Set_Size(kiv_os::THandle vfs_handle, int position, kiv_os::NFile_Seek seek_offset_type) {
	return vfs.Set_Size(vfs_handle, position, seek_offset_type);
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
	kiv_os::THandle handle;
	if (open_flag == kiv_os::NOpen_File::fmOpen_Always) {
		result = vfs.Open_File(path, attributes, handle);
	}
	else {
		result = vfs.Create_File(path, attributes, handle);
	}
	
	if (result == kiv_os::NOS_Error::Success) {
		handle = kiv_process::CProcess_Manager::Get_Instance().Save_Fd(handle);
		regs.rax.x = static_cast<decltype(regs.rax.x)>(handle);
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

	kiv_os::NOS_Error result = vfs.Close_File(vfs_handle);
	if (result == kiv_os::NOS_Error::Success) {
		kiv_process::CProcess_Manager::Get_Instance().Remove_Fd(proc_handle);
	}

	Set_Result(regs, result);
}

void Delete_File(kiv_hal::TRegisters &regs) {
	std::string path = reinterpret_cast<char *>(regs.rdx.r);

	kiv_os::NOS_Error result;
	result = vfs.Delete_File(path);

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
	result = vfs.Write_File(vfs_handle, buffer, buf_size, bytes_written);

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

	kiv_os::NOS_Error result = vfs.Read_File(vfs_handle, buffer, buf_size, bytes_read);

	regs.rax.r = bytes_read;
	Set_Result(regs, result);
}

void Set_Working_Dir(kiv_hal::TRegisters &regs) {
	char *path = reinterpret_cast<char *>(regs.rdx.r); // null terminated

	kiv_os::NOS_Error result = vfs.Set_Working_Directory(path);

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
	kiv_os::THandle *handle_pair = reinterpret_cast<kiv_os::THandle *>(regs.rdx.r);
	kiv_os::THandle in;
	kiv_os::THandle out;

	kiv_os::NOS_Error result = vfs.Create_Pipe(out, in);
	
	if (result == kiv_os::NOS_Error::Success) {
		handle_pair[0] = kiv_process::CProcess_Manager::Get_Instance().Save_Fd(out);
		handle_pair[1] = kiv_process::CProcess_Manager::Get_Instance().Save_Fd(in);
	}

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