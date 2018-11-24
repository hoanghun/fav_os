#include "..\api\api.h"
#include "parser.h"

extern "C" size_t __stdcall type(const kiv_hal::TRegisters &regs) { 

	const size_t buffer_size = 256;
	char buffer[buffer_size];
	size_t counter;

	const char* new_line = "\n";

	do {

		counter = kiv_os_rtl::Read_Line(regs, buffer, buffer_size);
		if (counter > 0) {

			if (counter == buffer_size) {
				counter--;
			}
			buffer[counter] = 0;	//udelame z precteneho vstup null-terminated retezec
			kiv_os_rtl::Print_Line(regs, new_line, strlen(new_line));
			kiv_os_rtl::Print_Line(regs, buffer, strlen(buffer));
		}

		
	} while (counter > 0);

	kiv_os_rtl::Exit(0);

	return 0;

};