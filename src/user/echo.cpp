#include "..\api\api.h"
#include "stdio.h"
#include "rtl.h"

extern "C" size_t __stdcall echo(const kiv_hal::TRegisters &regs) { 
	
	const char *text = (char*)(regs.rdx.r);

	const char* new_line = "\n";
	kiv_std_lib::Print_Line(regs, text, strlen(text));

	kiv_os_rtl::Exit(0);

	return 0; 

}
