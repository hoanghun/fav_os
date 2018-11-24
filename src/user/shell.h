#pragma once

#include "..\api\api.h"
#include "parser.h"

#include <vector>

extern "C" size_t __stdcall shell(const kiv_hal::TRegisters &regs);
void Prepare_For_Execution(std::vector<executable> &exes, const kiv_os::THandle sin, const kiv_os::THandle sout);
void Execute(std::vector<executable> &exes);


//nasledujici funkce si dejte do vlastnich souboru
//cd nemuze byt externi program, ale vestavny prikaz shellu!
//extern "C" size_t __stdcall type(const kiv_hal::TRegisters &regs) { return 0; };
//extern "C" size_t __stdcall md(const kiv_hal::TRegisters &regs) { return 0; }
//extern "C" size_t __stdcall rd(const kiv_hal::TRegisters &regs) { return 0; }
//extern "C" size_t __stdcall dir(const kiv_hal::TRegisters &regs) { return 0; }
//extern "C" size_t __stdcall echo(const kiv_hal::TRegisters &regs) { return 0; }
//extern "C" size_t __stdcall wc(const kiv_hal::TRegisters &regs) { return 0; }
//extern "C" size_t __stdcall sort(const kiv_hal::TRegisters &regs) { return 0; }
//extern "C" size_t __stdcall rgen(const kiv_hal::TRegisters &regs) { return 0; }
//extern "C" size_t __stdcall freq(const kiv_hal::TRegisters &regs) { return 0; }
extern "C" size_t __stdcall ps(const kiv_hal::TRegisters &regs) { return 0; }
extern "C" size_t __stdcall shutdown(const kiv_hal::TRegisters &regs) { return 0; }
