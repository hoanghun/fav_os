#include "..\api\api.h"
#include "stdio.h"
#include "rtl.h"

extern "C" size_t __stdcall echo(const kiv_hal::TRegisters &regs) { 
	
	size_t len = strlen((char*)(regs.rdi.r)) + 1;
	char *text = new char[len];
	strcpy_s(text, len, (char*)(regs.rdi.r));

	const char* new_line = "\n";
	kiv_std_lib::Print_Line(regs, new_line, strlen(new_line));
	kiv_std_lib::Print_Line(regs, text, strlen(text));

	delete text;

	kiv_os_rtl::Exit(0);

	return 0; 

}
