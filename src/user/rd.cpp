#include "..\api\api.h"
#include "rtl.h"
#include "common.h"
#include <vector>
#include <string>

bool Remove_Directory(const std::string dir_name, const kiv_hal::TRegisters &regs) {
	kiv_os::THandle handle;

	// TODO Otestovat jestli se mažou jen složky

	// Check if file exists and if it is a directory
	bool result = kiv_os_rtl::Open_File(dir_name.c_str(), kiv_os::NOpen_File::fmOpen_Always, kiv_os::NFile_Attributes::Directory, handle);

	if (result) {
		kiv_os_rtl::Close_Handle(handle);
		result = kiv_os_rtl::Delete_File(dir_name.c_str());
	}

	if (!result) {
		std::string err_msg;

		switch (kiv_os_rtl::Last_Error) {
			case kiv_os::NOS_Error::File_Not_Found:
				err_msg = "Direcotry '" + dir_name + "' does not exist.";
				break;

			case kiv_os::NOS_Error::Directory_Not_Empty:
				err_msg = "Directory '" + dir_name + "' is not empty.";
				break;

			case kiv_os::NOS_Error::Permission_Denied:
				err_msg = "Directory '" + dir_name + "' cannot be deleted.";
				break;

			default:
				err_msg = "Deletion of '" + dir_name + "' failed. Try again.";
				break;
		}

		kiv_os_rtl::Print_Line(regs, err_msg.c_str(), err_msg.size());
		return false;
	}

	return result;
}

extern "C" size_t __stdcall rd(const kiv_hal::TRegisters &regs) {
	std::vector<std::string> args;
	kiv_common::Parse_Arguments(regs, "md", args);

	int exit_code = EXIT_SUCCESS;

	// TODO Wrong nuber of parameters
	if (args.size() == 1) {
		std::string err_msg = "Invalid arguments.";
		kiv_os_rtl::Print_Line(regs, err_msg.c_str(), err_msg.length());
		exit_code = EXIT_FAILURE;
	}
	else {
		for (int i = 1; i < args.size(); i++) {
			// If creation fails -> set exit code to failure and continue
			if (!Remove_Directory(args.at(i), regs)) {
				exit_code = EXIT_FAILURE;
			}
		}
	}

	kiv_os_rtl::Exit(exit_code);
	return 0;
}