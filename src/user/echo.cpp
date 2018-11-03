#include "..\api\api.h"
#include "stdio.h"
#include "rtl.h"

extern "C" size_t __stdcall echo(const kiv_hal::TRegisters &regs) { 
	
	const size_t buffer_size = 256;
	char buffer[buffer_size];
	size_t counter;
	
	while ((counter = kiv_std_lib::Read_Line(regs, buffer, buffer_size)) > 0) {
		
		if (counter == buffer_size) {
			counter--;
		}

		buffer[counter] = 0;	//udelame z precteneho vstup null-terminated retezec

		const char* new_line = "\n";
		kiv_std_lib::Print_Line(regs, new_line, strlen(new_line));
		kiv_std_lib::Print_Line(regs, buffer, strlen(buffer));

	}

	//TODO presunout do rtl a udelat z toho crt0??
	kiv_os_rtl::Exit(0);

	return 0; 

}
