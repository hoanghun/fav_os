#include <string>
#include <cstdlib>

#include "..\api\api.h"
#include "rtl.h"

#include <iostream>

size_t _stdcall generate_floats(const kiv_hal::TRegisters &context) {
	kiv_hal::TRegisters regs = context;
	bool *run = reinterpret_cast<bool *>(context.rdi.r);

	std::string float_num;
	double number;
	
	while (*run) {
		number = static_cast<double>(rand()) / RAND_MAX;
		float_num = std::to_string(number);
		kiv_os_rtl::Print_Line(regs, float_num.c_str(), strlen(float_num.c_str()));
		//std::cout << float_num << std::endl;
	}

	char buffer[1] = { EOF };
	kiv_os_rtl::Print_Line(regs, buffer, sizeof(buffer));
	
	kiv_os_rtl::Exit(0);
		
	return 0;
}


extern "C" size_t __stdcall rgen(const kiv_hal::TRegisters &regs) {
	const size_t size = 0xFF;
	char buffer[size];
	size_t read, handle, tmp;
	bool run = true;

	kiv_os_rtl::Thread(generate_floats, &run, handle);
	do {
		read = kiv_os_rtl::Read_Line(regs, buffer, size);
	} while (read != 0);

	run = false;

	kiv_os_rtl::Wait_For(&handle, 1, tmp);
	kiv_os_rtl::Exit(0);

	return 0; 
}