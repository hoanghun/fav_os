#include "..\api\api.h"
#include "stdio.h"
#include "rtl.h"

#include <map>
#include <sstream>

extern "C" size_t __stdcall freq(const kiv_hal::TRegisters &regs) {
	std::map<char, int> freq_table;

	const size_t buffer_size = 256;
	char buffer[buffer_size];
	size_t counter = 1;

	while (counter) {
		counter = kiv_os_rtl::Stdin_Read(regs, buffer, buffer_size);
		for (int i = 0; i < counter; i++) { 
			auto result = freq_table.find(buffer[i]);
			if (result == freq_table.end()) {
				freq_table.emplace(buffer[i], 1);
			}
			else {
				result->second += 1;
			}	
		}
	}

	const char* new_line = "\n";
	for (const auto &item : freq_table) {
		kiv_os_rtl::Stdout_Print(regs, new_line, strlen(new_line));
		std::stringstream s;
		s.str("");

		s << printf("0x%hhx : %d", item.first, item.second);
	}

	kiv_os_rtl::Exit(0);

	return 0;
}