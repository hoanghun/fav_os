#include "..\api\api.h"
#include "rtl.h"

#include <string>
#include <iostream>

const std::string proc_root = "proc:\\";
const size_t buffer_size = 0xFF;
const char *new_line = "\n";
const char *tab = "\t";
const char *header = "\tNAME           PID";
const char *hline =  "\t=========   ======";


extern "C" size_t __stdcall tasklist(const kiv_hal::TRegisters &regs) {
	kiv_os::THandle root_handle, file_handle;
	kiv_os::TDir_Entry entry;
	size_t read = 0, file_read = 0;
	char buffer[buffer_size];
	bool ok_status = kiv_os_rtl::Open_File(proc_root.c_str(), 
		kiv_os::NOpen_File::fmOpen_Always, kiv_os::NFile_Attributes::Read_Only, root_handle);

	if (ok_status) {
		kiv_os_rtl::Set_Working_Dir(proc_root.c_str());
		kiv_os_rtl::Stdout_Print(regs, new_line, strlen(new_line));
		kiv_os_rtl::Stdout_Print(regs, header, strlen(header));
		kiv_os_rtl::Stdout_Print(regs, new_line, strlen(new_line));
		kiv_os_rtl::Stdout_Print(regs, hline, strlen(hline));
		
		while (kiv_os_rtl::Read_File(root_handle, &entry, sizeof(entry), read) && read != 0) {
			if (kiv_os_rtl::Open_File(entry.file_name, kiv_os::NOpen_File::fmOpen_Always, 
				kiv_os::NFile_Attributes::Read_Only, file_handle)) {

				kiv_os_rtl::Read_File(file_handle, buffer, buffer_size, file_read);
				if (file_read) {
					kiv_os_rtl::Stdout_Print(regs, new_line, strlen(new_line));
					kiv_os_rtl::Stdout_Print(regs, tab, strlen(tab));
					kiv_os_rtl::Stdout_Print(regs, buffer, file_read);
				}
				
				kiv_os_rtl::Close_Handle(file_handle);
			}
		}
	}
	kiv_os_rtl::Close_Handle(root_handle);

	kiv_os_rtl::Exit(0);
	return 0; 
}