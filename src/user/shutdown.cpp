#include "..\api\api.h"
#include "rtl.h"
#include <iostream>

extern "C" size_t __stdcall shutdown(const kiv_hal::TRegisters &regs) {

	/*const char *text = "System shutdown in process ...";
	kiv_os_rtl::Stdout_Print(regs, text, strlen(text));*/
	kiv_os_rtl::Shutdown();
	kiv_os_rtl::Exit(0);

	return 0;
}