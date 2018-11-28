#include "..\api\api.h"
#include "rtl.h"

#include <iostream>
#include <string>
#include <cstdlib>

bool run;
const size_t size = 0xFF;
 
size_t _stdcall sigterm_handler(const kiv_hal::TRegisters &context) {
	run = false;

	return 0;
}

size_t _stdcall generate_floats(const kiv_hal::TRegisters &context) {
	kiv_hal::TRegisters regs = context;

	std::string float_num;
	double number;
	
	while (run) {
		number = static_cast<double>(rand()) / RAND_MAX;
		float_num = std::to_string(number);
		kiv_os_rtl::Stdout_Print(regs, float_num.c_str(), strlen(float_num.c_str()));
		kiv_os_rtl::Stdout_Print(regs, "\n", strlen("\n"));
	}

 	kiv_os_rtl::Exit(0);

	return 0;
}


extern "C" size_t __stdcall rgen(const kiv_hal::TRegisters &regs) {
	char buffer[size];
	size_t read, handle, signaled;
	run = true;
	
	if (!kiv_os_rtl::Register_Terminate_Signal_Handler(sigterm_handler)) {
		
	}

	kiv_os_rtl::Thread(generate_floats, NULL, handle);

	do {
		read = kiv_os_rtl::Stdin_Read(regs, buffer, size);
	} while (read != 0 && run);

	run = false;
	kiv_os_rtl::Wait_For(&handle, 1, signaled);
	kiv_os_rtl::Exit(0);

	return 0; 
}