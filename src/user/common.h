#pragma once
#include "../api/api.h"
#include "rtl.h"
#include <vector>
#include <string>

namespace kiv_common {

	void Parse_Arguments(const kiv_hal::TRegisters &regs, std::string prog_name, std::vector<std::string> &args);
}