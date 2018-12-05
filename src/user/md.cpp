#include "..\api\api.h"
#include "rtl.h"
#include "common.h"
#include <vector>
#include <string>

bool Create_Directory(const std::string dir_name, const kiv_hal::TRegisters &regs){
	kiv_os::THandle handle;

	// Check if file or directory with that name already exists
	bool exists = kiv_os_rtl::Open_File(dir_name.c_str(), kiv_os::NOpen_File::fmOpen_Always, (kiv_os::NFile_Attributes)0, handle);
	if (exists) {
		kiv_os_rtl::Close_Handle(handle);
		std::string error = "A subdirectory or file " + dir_name + " exists.\n";
		kiv_os_rtl::Stdout_Print(regs, error.c_str(), error.length());
		return false;
	}

	// Create new directory
	bool result = kiv_os_rtl::Open_File(dir_name.c_str(), (kiv_os::NOpen_File)0, kiv_os::NFile_Attributes::Directory, handle);
	if (result) {
		kiv_os_rtl::Close_Handle(handle);
		return true;
	}
	else {
		std::string err_msg = "Directory " + dir_name + " couldn't be created\n";
		kiv_os_rtl::Stdout_Print(regs, err_msg.c_str(), err_msg.length());
		return false;
	}
}

extern "C" size_t __stdcall md(const kiv_hal::TRegisters &regs) {

	std::vector<std::string> args;
	kiv_common::Parse_Arguments(regs, "md", args);

	int exit_code = EXIT_SUCCESS;

	if (args.size() == 1) {
		std::string err_msg = "The syntax of the command is incorrect.\n";
		kiv_os_rtl::Stdout_Print(regs, err_msg.c_str(), err_msg.length());
		exit_code = EXIT_FAILURE;
	}
	else {
		for (int i = 1; i < args.size(); i++) {
			// If creation fails -> set exit code to failure and continue
			if (!Create_Directory(args.at(i), regs)) {
				exit_code = EXIT_FAILURE;
			}
		}
	}

	kiv_os_rtl::Exit(exit_code);
	return 0;
}