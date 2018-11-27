#include "..\api\api.h"
#include "parser.h"

#include <string>

extern "C" size_t __stdcall wc(const kiv_hal::TRegisters &regs) { 
	
	const size_t buffer_size = 256;
	char buffer[buffer_size];
	size_t counter;

	const char new_line = '\n';

	size_t lines_count = 0;
	do {

		counter = kiv_os_rtl::Read_Line(regs, buffer, buffer_size);
		if (counter > 0) {
			
			for (int i = 0; i < counter; i++) {
				if (buffer[i] == new_line) {
					lines_count++;
				}
			}

		}

	} while (counter > 0);

	std::string output = "\nWords count: " + std::to_string(lines_count);
	kiv_os_rtl::Print_Line(regs, output.c_str(), output.length());

	kiv_os_rtl::Exit(0);

	return 0;

}