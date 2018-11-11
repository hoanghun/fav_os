#include "rtl.h"

std::atomic<kiv_os::NOS_Error> kiv_os_rtl::Last_Error;

kiv_hal::TRegisters Prepare_SysCall_Context(kiv_os::NOS_Service_Major major, uint8_t minor) {
	kiv_hal::TRegisters regs;
	regs.rax.h = static_cast<uint8_t>(major);
	regs.rax.l = minor;
	return regs;
}


bool kiv_os_rtl::Read_File(const kiv_os::THandle file_handle, char* const buffer, const size_t buffer_size, size_t &read) {
	kiv_hal::TRegisters regs =  Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Read_File));
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

	kiv_hal::TRegisters regs;
	regs.rax.h = static_cast<uint8_t>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<uint8_t>(kiv_os::NOS_Process::Exit);
	regs.rcx.r = 0;

	kiv_os::Sys_Call(regs);
}

bool kiv_os_rtl::Clone(const char *prog_name, const char *args, kiv_os::THandle in, kiv_os::THandle out, size_t &handle) {

	kiv_hal::TRegisters regs;

	regs.rbx.e = in;
	regs.rbx.e = (regs.rbx.e << 16) | out;

	regs.rax.h = static_cast<uint8_t>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<uint8_t>(kiv_os::NOS_Process::Clone);
	regs.rcx.r = static_cast<uint8_t>(kiv_os::NClone::Create_Process);

	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(prog_name);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdx.r)>(args);

	kiv_os::Sys_Call(regs);

	handle = regs.rax.r;

	//TODO
	return true;
}

bool kiv_os_rtl::Thread(const char *prog_name, const char *data, size_t &handle) {

	kiv_hal::TRegisters regs;

	regs.rax.h = static_cast<uint8_t>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<uint8_t>(kiv_os::NOS_Process::Clone);
	regs.rcx.r = static_cast<uint8_t>(kiv_os::NClone::Create_Thread);

	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(prog_name);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdx.r)>(data);

	kiv_os::Sys_Call(regs);

	handle = regs.rax.r;

	//TODO
	return true;
}

bool kiv_os_rtl::Wait_For(const size_t *handles, const size_t handles_count, size_t &signaled) {


	kiv_hal::TRegisters regs;
	regs.rax.h = static_cast<uint8_t>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<uint8_t>(kiv_os::NOS_Process::Wait_For);

	regs.rdx.r = reinterpret_cast<size_t>(handles);
	regs.rcx.r = handles_count;

	kiv_os::Sys_Call(regs);

	//TODO
	return true;
}

bool kiv_os_rtl::Register_Terminate_Signal_Handler(const kiv_os::TThread_Proc *handler) {

	kiv_hal::TRegisters regs;
	regs.rax.h = static_cast<uint8_t>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<uint8_t>(kiv_os::NOS_Process::Register_Signal_Handler);

	regs.rdx.r = reinterpret_cast<size_t>(handler);
	regs.rcx.r = static_cast<uint64_t>(kiv_os::NSignal_Id::Terminate);

	kiv_os::Sys_Call(regs);

	//TODO
	return true;

}

bool kiv_os_rtl::Read_Exit_Code(const kiv_os::TThread_Proc *handler, int &exit_code) {

	kiv_hal::TRegisters regs;
	regs.rax.h = static_cast<uint8_t>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<uint8_t>(kiv_os::NOS_Process::Read_Exit_Code);

	regs.rdx.r = reinterpret_cast<size_t>(handler);

	kiv_os::Sys_Call(regs);

	exit_code = regs.rcx.x;

	//TODO
	return true;

}