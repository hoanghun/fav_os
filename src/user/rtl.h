#pragma once

#include "..\api\api.h"
#include <atomic>

namespace kiv_os_rtl {

	extern std::atomic<kiv_os::NOS_Error> Last_Error;

	bool Read_File(const kiv_os::THandle file_handle, char* const buffer, const size_t buffer_size, size_t &read);
		//zapise do souboru identifikovaneho deskriptor data z buffer o velikosti buffer_size a vrati pocet zapsanych dat ve written
		//vraci true, kdyz vse OK
		//vraci true, kdyz vse OK

	bool Write_File(const kiv_os::THandle file_handle, const char *buffer, const size_t buffer_size, size_t &written);
	//zapise do souboru identifikovaneho deskriptor data z buffer o velikosti buffer_size a vrati pocet zapsanych dat ve written
	//vraci true, kdyz vse OK
	//vraci true, kdyz vse OK

	void Exit(const int exit_code);

	bool Clone(const char *prog_name, const char *args, kiv_os::THandle in, kiv_os::THandle out, size_t &handle);

	bool Thread(const char *prog_name, const char *data, size_t &handle);

	bool Wait_For(const size_t *handles, const size_t handles_count, size_t &signaled);

	bool Register_Terminate_Signal_Handler(const kiv_os::TThread_Proc *handler);

	bool Read_Exit_Code(const kiv_os::TThread_Proc *handler, int &exit_code);

}