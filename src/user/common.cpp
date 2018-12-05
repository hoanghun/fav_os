#include "common.h"

namespace kiv_common {
	void Parse_Arguments(const kiv_hal::TRegisters &regs, std::string prog_name, std::vector<std::string> &args) {
		std::string args_line = reinterpret_cast<char *>(regs.rdi.r);

		// Add name of the program
		args.push_back(prog_name);

		// Parse arguments
		size_t len = args_line.length();
		bool qot = false;
		int arglen;
		for (int i = 0; i < len; i++) {
			int start = i;
			if (args_line[i] == '\"') {
				qot = true;
			}
			if (qot) {
				i++;
				start++;
				while (i < len && args_line[i] != '\"') {
					i++;
				}
				if (i < len) {
					qot = false;
				}
				arglen = i - start;
				i++;
			}
			else {
				while (i < len && args_line[i] != ' ') {
					i++;
				}
				arglen = i - start;
			}
			args.push_back(args_line.substr(start, arglen));
		}
	}

	bool Get_Position(const kiv_os::THandle handle, size_t &position) {
		return kiv_os_rtl::Seek(handle, 0, kiv_os::NFile_Seek::Beginning, kiv_os::NFile_Seek::Get_Position, position);
	}

	bool Set_Position(const kiv_os::THandle handle, size_t new_position, kiv_os::NFile_Seek pos_type) {
		size_t position;
		return kiv_os_rtl::Seek(handle, new_position, pos_type, kiv_os::NFile_Seek::Set_Position, position);
	}

	bool Set_Size(const kiv_os::THandle handle, size_t new_position, kiv_os::NFile_Seek pos_type) {
		size_t position;
		return kiv_os_rtl::Seek(handle, new_position, pos_type, kiv_os::NFile_Seek::Set_Size, position);
	}
}