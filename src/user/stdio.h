#pragma once

#include "..\api\api.h"

namespace kiv_std_lib {
	size_t Print_Line(const kiv_hal::TRegisters &regs, const char *buffer, size_t size);
	size_t Read_Line(const kiv_hal::TRegisters &regs, char* const buffer, size_t size);
}

