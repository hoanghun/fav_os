#pragma once
#include "../api/api.h"
#include "rtl.h"
#include <vector>
#include <string>

namespace kiv_common {
	void Parse_Arguments(const kiv_hal::TRegisters &regs, std::string prog_name, std::vector<std::string> &args);
	bool Get_Position(const kiv_os::THandle handle, size_t &position);
	bool Set_Position(const kiv_os::THandle handle, size_t new_position, kiv_os::NFile_Seek pos_type);
	bool Set_Size(const kiv_os::THandle handle, size_t new_position, kiv_os::NFile_Seek pos_type);
}