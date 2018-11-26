#include "..\api\api.h"
#include "rtl.h"

#include <iostream>
#include <string>
#include <cstdlib>

bool run; // hack, melo by se posilat pres argumenty, z nejakyho duvodu padlo 1 ze 100 napr?
const size_t size = 0xFF;

size_t _stdcall generate_floats(const kiv_hal::TRegisters &context) {
	kiv_hal::TRegisters regs = context;

	std::string float_num;
	double number;
	
	while (run) {
		number = static_cast<double>(rand()) / RAND_MAX;
		float_num = std::to_string(number);
		kiv_os_rtl::Print_Line(regs, float_num.c_str(), strlen(float_num.c_str()));
	}

 	kiv_os_rtl::Exit(0);

	return 0;
}


extern "C" size_t __stdcall rgen(const kiv_hal::TRegisters &regs) {
	char buffer[size];
	size_t read, handle, signaled;
	run = true;

	kiv_os_rtl::Thread(generate_floats, NULL, handle);

	do {
		read = kiv_os_rtl::Read_Line(regs, buffer, size);
	} while (read != 0);

	run = false;
	kiv_os_rtl::Wait_For(&handle, 1, signaled);
	kiv_os_rtl::Exit(0);

	return 0; 
}