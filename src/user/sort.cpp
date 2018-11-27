#include "..\api\api.h"
#include "parser.h"

#include <vector>
#include <sstream>
#include <algorithm>

extern "C" size_t __stdcall sort(const kiv_hal::TRegisters &regs) { 

	const size_t buffer_size = 256;
	char buffer[buffer_size];
	size_t counter;

	const char new_line = '\n';

	std::vector<std::string> output;

	std::ostringstream oss;

	do {


		counter = kiv_os_rtl::Read_Line(regs, buffer, buffer_size);
		if (counter > 0) {

			for (int i = 0; i < counter; i++) {
				oss << buffer[i];
				if (buffer[i] == new_line) {
					output.push_back(oss.str());
					oss.str("");
				}
			}
			
		}

	} while (counter > 0);
	output.push_back(oss.str());


	std::sort(output.begin(), output.end());
	for (const std::string &line : output) {
		const char *out_line = line.c_str();
		kiv_os_rtl::Print_Line(regs, out_line, strlen(out_line));
	}

	kiv_os_rtl::Exit(0);

	return 0;

}