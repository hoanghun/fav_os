#include "rtl.h"

std::atomic<kiv_os::NOS_Error> kiv_os_rtl::Last_Error;

kiv_hal::TRegisters Prepare_SysCall_Context(kiv_os::NOS_Service_Major major, uint8_t minor) {
	kiv_hal::TRegisters regs;
	regs.rax.h = static_cast<uint8_t>(major);
	regs.rax.l = minor;
	return regs;
}


bool kiv_os_rtl::Read_File(const kiv_os::THandle file_handle, void* const buffer, const size_t buffer_size, size_t &read) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Read_File));
	regs.rdx.x = static_cast<decltype(regs.rdx.x)>(file_handle);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(buffer);
	regs.rcx.r = buffer_size;	

	const bool result = kiv_os::Sys_Call(regs);
	read = regs.rax.r;
	return result;
}

bool kiv_os_rtl::Write_File(const kiv_os::THandle file_handle, const char *buffer, const size_t buffer_size, size_t &written) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Write_File));
	regs.rdx.x = static_cast<decltype(regs.rdx.x)>(file_handle);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(buffer);
	regs.rcx.r = buffer_size;

	const bool result = kiv_os::Sys_Call(regs);
	written = regs.rax.r;
	return result;
}

void kiv_os_rtl::Exit(const int exit_code) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Exit));
	regs.rcx.r = 0;

	bool result = kiv_os::Sys_Call(regs);
}

bool kiv_os_rtl::Clone(const char *prog_name, const char *args, kiv_os::THandle in, kiv_os::THandle out, size_t &handle) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Clone));
	regs.rbx.e = in;
	regs.rbx.e = (regs.rbx.e << 16) | out;
	regs.rcx.r = static_cast<uint8_t>(kiv_os::NClone::Create_Process);
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(prog_name);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdx.r)>(args);

	bool result = kiv_os::Sys_Call(regs);
	handle = regs.rax.r;
	return result;
}

bool kiv_os_rtl::Thread(const kiv_os::TThread_Proc func, const void *data, size_t &handle) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Clone));
	regs.rcx.r = static_cast<uint8_t>(kiv_os::NClone::Create_Thread);
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(func);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdx.r)>(data);

	bool result = kiv_os::Sys_Call(regs);
	handle = regs.rax.r;
	return result;
}

bool kiv_os_rtl::Wait_For(const size_t *handles, const size_t handles_count, size_t &signaled) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Wait_For));
	regs.rdx.r = reinterpret_cast<size_t>(handles);
	regs.rcx.r = handles_count;

	bool result = kiv_os::Sys_Call(regs);
	signaled = regs.rax.r;

	return result;
}

bool kiv_os_rtl::Register_Terminate_Signal_Handler(kiv_os::TThread_Proc handler) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Register_Signal_Handler));
	regs.rdx.r = reinterpret_cast<size_t>(handler);
	regs.rcx.r = static_cast<uint64_t>(kiv_os::NSignal_Id::Terminate);

	bool result = kiv_os::Sys_Call(regs);

	return result;
}

bool kiv_os_rtl::Read_Exit_Code(const size_t handler, int &exit_code) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Read_Exit_Code));
	regs.rdx.r = handler;

	bool result = kiv_os::Sys_Call(regs);
	exit_code = regs.rcx.x;
	return  result;
}

bool kiv_os_rtl::Open_File(const char * file_name, const kiv_os::NOpen_File flags, const kiv_os::NFile_Attributes attributes, kiv_os::THandle &handle) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Open_File));
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(file_name);
	regs.rcx.r = static_cast<size_t>(flags);
	regs.rdi.r = static_cast<size_t>(attributes);

	bool result = kiv_os::Sys_Call(regs);
	handle = regs.rax.x;
	return result;
}

bool kiv_os_rtl::Seek(const kiv_os::THandle handle, const size_t new_position, kiv_os::NFile_Seek pos_type, size_t &position) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Seek));
	regs.rdx.x = handle;
	regs.rdi.r = new_position;
	regs.rcx.l = static_cast<uint8_t>(pos_type);

	bool result = kiv_os::Sys_Call(regs);
	position = regs.rax.x;
	return result;
}

bool kiv_os_rtl::Close_Handle(const kiv_os::THandle handle) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Close_Handle));
	regs.rdx.x = handle;
	
	bool result = kiv_os::Sys_Call(regs);
	return result;
}

bool kiv_os_rtl::Delete_File(const char *file_name) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Delete_File));
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(file_name);

	bool result = kiv_os::Sys_Call(regs);
	return result;

}

bool kiv_os_rtl::Set_Working_Dir(const char *path) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Set_Working_Dir));
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(path);

	bool result = kiv_os::Sys_Call(regs);
	return result;

}

bool kiv_os_rtl::Get_Working_Dir(char* const buffer, const size_t buffer_size, size_t &read) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Get_Working_Dir));
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(buffer);
	regs.rcx.r = buffer_size;

	const bool result = kiv_os::Sys_Call(regs);
	read = regs.rax.r;
	return result;
}

bool kiv_os_rtl::Create_Pipe(kiv_os::THandle &in, kiv_os::THandle &out) {
	kiv_os::THandle pipes[2] = { 0 };
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Create_Pipe));
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(pipes);

	const bool result = kiv_os::Sys_Call(regs);
	out = pipes[0];
	in = pipes[1];

	return result;
}

size_t kiv_os_rtl::Stdout_Print(const kiv_hal::TRegisters &regs, const char *buffer, size_t size) {
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	size_t printed;

	kiv_os_rtl::Write_File(std_out, buffer, size, printed);
	return printed;
}

size_t kiv_os_rtl::Stdin_Read(const kiv_hal::TRegisters &regs, char* const buffer, size_t size) {
	const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	size_t read;

	kiv_os_rtl::Read_File(std_in, buffer, size, read); // nemá cenu kontrolovat
	return read;
}

void kiv_os_rtl::Shutdown() {

	kiv_hal::TRegisters regs;

	regs.rax.h = static_cast<uint8_t>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<uint8_t>(kiv_os::NOS_Process::Shutdown);

	kiv_os::Sys_Call(regs);
}