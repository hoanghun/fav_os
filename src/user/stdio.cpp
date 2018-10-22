#include "stdio.h"
#include "rtl.h"

namespace kiv_std_lib {
	size_t Print_Line(const kiv_hal::TRegisters &regs, const char *buffer, size_t size) {
		const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
		size_t printed;

		if (kiv_os_rtl::Write_File(std_out, buffer, size, printed)) {
			return printed;
		}

		return -1; // something went wrong? didn't print anything?
	}

	size_t Read_Line(const kiv_hal::TRegisters &regs, char* const buffer, size_t size) {
		const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
		size_t read;

		if (kiv_os_rtl::Read_File(std_in, buffer, size, read)) {
			return read;
		}

		return -1;
	}
}
