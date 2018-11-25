#include "..\api\api.h"
#include "parser.h"

#include <sstream>

extern "C" size_t __stdcall type(const kiv_hal::TRegisters &regs) { 

	//TODO pridat chyby pokud spusteno s pajpou atd.

	const size_t buffer_size = 256;
	char buffer[buffer_size];
	size_t counter;

	const char* new_line = "\n";
	
	size_t len = strlen((char*)(regs.rdi.r)) + 1;

	if (len == 1) {
		const char *error = "\n TYPE invalid arguments.";
		kiv_os_rtl::Print_Line(regs, error, strlen(error));
		kiv_os_rtl::Exit(0);
		return 0;
	}

	std::string args = std::string(reinterpret_cast<const char*>(regs.rdi.r));

	size_t args_count = 0;
	std::string file_in;
	std::istringstream tokenStream(args);
	
	while (std::getline(tokenStream, file_in, ' ')) {
		args_count++;
	}

	if (args_count != 1) {
		const char *error = "\n TYPE invalid arguments.";
		kiv_os_rtl::Print_Line(regs, error, strlen(error));
		kiv_os_rtl::Exit(0);
		return 0;
	}

	kiv_os::THandle handle = 0;
	if (kiv_os_rtl::Open_File(file_in.c_str(), kiv_os::NOpen_File::fmOpen_Always, kiv_os::NFile_Attributes::Read_Only, handle)) {
		//TODO regs.rax.x = handle;
	}
	else {
		const std::string error = "\n TYPE '" + file_in + "' file not found.";
		kiv_os_rtl::Print_Line(regs, error.c_str(), error.length());
		kiv_os_rtl::Exit(0);
		return 0;
	}

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