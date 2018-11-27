#pragma once

#include "..\api\api.h"
#include <atomic>

namespace kiv_os_rtl {

	extern std::atomic<kiv_os::NOS_Error> Last_Error;

	bool Read_File(const kiv_os::THandle file_handle, void* const buffer, const size_t buffer_size, size_t &read);
		//zapise do souboru identifikovaneho deskriptor data z buffer o velikosti buffer_size a vrati pocet zapsanych dat ve written
		//vraci true, kdyz vse OK
		//vraci true, kdyz vse OK

	bool Write_File(const kiv_os::THandle file_handle, const char *buffer, const size_t buffer_size, size_t &written);
	//zapise do souboru identifikovaneho deskriptor data z buffer o velikosti buffer_size a vrati pocet zapsanych dat ve written
	//vraci true, kdyz vse OK
	//vraci true, kdyz vse OK

	void Exit(const int exit_code);

	bool Clone(const char *prog_name, const char *args, kiv_os::THandle in, kiv_os::THandle out, size_t &handle);

	bool Thread(const kiv_os::TThread_Proc func, const void *data, size_t &handle);

	bool Wait_For(const size_t *handles, const size_t handles_count, size_t &signaled);

	bool Register_Terminate_Signal_Handler(kiv_os::TThread_Proc handler);

	bool Read_Exit_Code(const size_t handler, int &exit_code);

	bool Open_File(const char * file_name, const kiv_os::NOpen_File flags, const kiv_os::NFile_Attributes attributes, kiv_os::THandle &handle);

	bool Seek(const kiv_os::THandle handle, const size_t new_position, kiv_os::NFile_Seek pos_type, size_t &position);

	bool Close_Handle(const kiv_os::THandle handle);

	bool Delete_File(const char *file_name);

	bool Set_Working_Dir(const char *path);

	bool Get_Working_Dir(char* const buffer, const size_t buffer_size, size_t &read);

	bool Create_Pipe(kiv_os::THandle &in, kiv_os::THandle &out);

	size_t Print_Line(const kiv_hal::TRegisters &regs, const char *buffer, size_t size);

	size_t Read_Line(const kiv_hal::TRegisters &regs, char* const buffer, size_t size);

	void Shutdown();

}