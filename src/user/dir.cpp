#include "..\api\api.h"
#include "rtl.h"
#include "common.h"
#include <vector>
#include <string>
#include <iostream>
bool Print_Directory(const std::string dir_name, const kiv_hal::TRegisters &regs) {

	// TODO Otestovat výpis souborù (ne složek)

	kiv_os::THandle handle;
	// Check if file or directory with that name already exists
	bool exists = kiv_os_rtl::Open_File(dir_name.c_str(), kiv_os::NOpen_File::fmOpen_Always, kiv_os::NFile_Attributes::Read_Only, handle);

	std::string line;
	if (exists) {
		line = "\nContent of directory '" + dir_name + "':\n";
		kiv_os_rtl::Print_Line(regs, line.c_str(), line.size());

		kiv_os::TDir_Entry entry;
		size_t bytes_read;
		while (kiv_os_rtl::Read_File(handle, &entry, sizeof(kiv_os::TDir_Entry), bytes_read) && (bytes_read != 0)) {
			if (entry.file_attributes & (uint8_t)kiv_os::NFile_Attributes::Directory) {
				line = "<DIR>\t";
			}
			else {
				line = "\t";
			}
			line += entry.file_name;
			line += "\n";
			kiv_os_rtl::Print_Line(regs, line.c_str(), line.size());
		}

		kiv_os_rtl::Close_Handle(handle);
	}
	else {
		line = "\nDirectory '" + dir_name + "' doesn't exist.\n";
		kiv_os_rtl::Print_Line(regs, line.c_str(), line.size());
	}

	return exists;
}

extern "C" size_t __stdcall dir(const kiv_hal::TRegisters &regs) {
	std::vector<std::string> args;
	kiv_common::Parse_Arguments(regs, "dir", args);

	int exit_code = EXIT_SUCCESS;

	// If there are no arguments -> print current directory
	if (args.size() == 1) {
		args.push_back(".");
	}

	for (std::vector<std::string>::iterator dir = (args.begin() + 1); dir != args.end(); ++dir) {
		// If reading fails -> set exit code to failure and continue
		if (!Print_Directory(*dir, regs)) {
			exit_code = EXIT_FAILURE;
		}
	}

	kiv_os_rtl::Exit(exit_code);
	return 0;
}