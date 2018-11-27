#include "..\api\api.h"
#include "parser.h"
#include "common.h"

#include <sstream>
#include <vector>
#include <string>

static const char *err_no_args_msg = "\nThe syntax of the command is incorrect.";
static const char *err_invalid_file = "\nThe system cannot find the file specified.";
static const char *new_line = "\n";
static const char *file_intro = "file:\t";
static const char *con = "stdio:/stdin";

void print_file_name(kiv_hal::TRegisters regs, const char *file_name) {
	kiv_os_rtl::Print_Line(regs, new_line, strlen(new_line));
	//kiv_os_rtl::Print_Line(regs, file_intro, strlen(file_intro));
	kiv_os_rtl::Print_Line(regs, file_name, strlen(file_name));
	kiv_os_rtl::Print_Line(regs, new_line, strlen(new_line));
	//kiv_os_rtl::Print_Line(regs, new_line, strlen(new_line));
}

void print_content(const kiv_hal::TRegisters &regs, const kiv_os::THandle handle) {
	size_t counter;
	const size_t buffer_size = 0xFF;
	char buffer[buffer_size];

	do {
		kiv_os_rtl::Print_Line(regs, new_line, strlen(new_line));
		kiv_os_rtl::Read_File(handle, buffer, buffer_size, counter);
		if (counter > 0) {
			kiv_os_rtl::Print_Line(regs, new_line, strlen(new_line));
			if (counter == buffer_size) {
				counter--;
			}
			buffer[counter] = 0;	//udelame z precteneho vstup null-terminated retezec
			kiv_os_rtl::Print_Line(regs, buffer, strlen(buffer));
			//kiv_os_rtl::Print_Line(regs, new_line, strlen(new_line));
		}

	} while (counter > 0);
}
const char *map(const char *file_name) {
	const char *console = "con";
	if (!strcmp(file_name, console)) {
		return con;
	}

	return file_name;
}

extern "C" size_t __stdcall type(const kiv_hal::TRegisters &regs) { 
	std::vector<std::string> args;
	kiv_os::THandle handle;
	bool more_files = false;
	kiv_common::Parse_Arguments(regs, "type", args);

	if (args.size() == 1) {
		kiv_os_rtl::Print_Line(regs, err_no_args_msg, strlen(err_no_args_msg));
		kiv_os_rtl::Print_Line(regs, new_line, strlen(new_line));
		kiv_os_rtl::Exit(-1);
	}

	if (args.size() > 2) {
		more_files = true;
	}

	auto file = args.begin();
	std::advance(file, 1);
	for (; file != args.end(); ++file) {
		const char *file_name_tmp = map((*file).c_str());
		if (kiv_os_rtl::Open_File(file_name_tmp, kiv_os::NOpen_File::fmOpen_Always,
			kiv_os::NFile_Attributes::Read_Only, handle)) {
			if (more_files) {
				print_file_name(regs, file_name_tmp);
			}
			
			print_content(regs, handle);
		}
		else {
			kiv_os_rtl::Print_Line(regs, err_invalid_file, strlen(err_invalid_file));
		}

		//kiv_os_rtl::Print_Line(regs, new_line, strlen(new_line));
	}

	kiv_os_rtl::Exit(0);
	return 0;
};