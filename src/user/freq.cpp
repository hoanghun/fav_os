#include "..\api\api.h"
#include "stdio.h"
#include "rtl.h"

#include <map>
#include <sstream>
#include <iostream>

extern "C" size_t __stdcall freq(const kiv_hal::TRegisters &regs) {
	
	std::map<char, int> freq_table;

	const size_t buffer_size = 256;
	char buffer[buffer_size];
	size_t counter = 0;

	bool run = true;
	while (run) {
		//std::cout << "jdu si neco precist" << std::endl;
		//counter = kiv_os_rtl::Read_Line(regs, buffer, buffer_size);
		//std::cout << "neco jsem si precist" << std::endl;
		for (int i = 0; i < counter; i++) {
			if (buffer[i] == kiv_hal::NControl_Codes::NUL || buffer[i] == kiv_hal::NControl_Codes::EOT || buffer[i] == kiv_hal::NControl_Codes::ETX || EOF) {
				run = false;
				break;
			}
			else {
				auto result = freq_table.find(buffer[i]);
				if (result == freq_table.end()) {
					freq_table.emplace(buffer[i], 1);
				}
				else {
					result->second += 1;
				}
			}
		}
		//std::cout << "2jdu si neco precist" << std::endl;
	}

	for (const auto &item : freq_table) {

		std::stringstream s;
		s.str("");

		s << printf("0x%hhx : %d\n", item.first, item.second);

		const char * line = s.str().c_str();
		kiv_os_rtl::Print_Line(regs, line, strlen(line));

	}

	return 0;
}