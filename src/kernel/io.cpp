#include "io.h"
#include "kernel.h"
#include "handles.h"
#include "vfs.h"



size_t Read_Line_From_Console(char *buffer, const size_t buffer_size) {
	kiv_hal::TRegisters registers;

	size_t pos = 0;
	while (pos < buffer_size) {
		//read char
		registers.rax.h = static_cast<decltype(registers.rax.l)>(kiv_hal::NKeyboard::Read_Char);
		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Keyboard, registers);
		
		char ch = registers.rax.l;

		//osetrime zname kody
		switch (static_cast<kiv_hal::NControl_Codes>(ch)) {
			case kiv_hal::NControl_Codes::BS: {
					//mazeme znak z bufferu
					if (pos > 0) pos--;

					registers.rax.h = static_cast<decltype(registers.rax.l)>(kiv_hal::NVGA_BIOS::Write_Control_Char);
					registers.rdx.l = ch;
					kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, registers);
				}
				break;

			case kiv_hal::NControl_Codes::LF:  break;	//jenom pohltime, ale necteme
			case kiv_hal::NControl_Codes::NUL:			//chyba cteni?
			case kiv_hal::NControl_Codes::CR:  return pos;	//docetli jsme az po Enter


			default: buffer[pos] = ch;
					 pos++;	
					 registers.rax.h = static_cast<decltype(registers.rax.l)>(kiv_hal::NVGA_BIOS::Write_String);
					 registers.rdx.r = reinterpret_cast<decltype(registers.rdx.r)>(&ch);
					 registers.rcx.r = 1;
					 kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, registers);
					 break;
		}
	}

	return pos;
}

void Open_File(kiv_hal::TRegisters &regs, kiv_vfs::CVirtual_File_System &vfs) {
	// TODO
}

void Close_File(kiv_hal::TRegisters &regs, kiv_vfs::CVirtual_File_System &vfs) {
	// TODO
}

void Delete_File(kiv_hal::TRegisters &regs, kiv_vfs::CVirtual_File_System &vfs) {
	// TODO
}

void Write_File(kiv_hal::TRegisters &regs, kiv_vfs::CVirtual_File_System &vfs) {
	// TODO
}

void Read_File(kiv_hal::TRegisters &regs, kiv_vfs::CVirtual_File_System &vfs) {
	// TODO
}

void Seek(kiv_hal::TRegisters &regs, kiv_vfs::CVirtual_File_System &vfs) {
	// TODO
}

void Set_Working_Dir(kiv_hal::TRegisters &regs, kiv_vfs::CVirtual_File_System &vfs) {
	// TODO
}

void Get_Working_Dir(kiv_hal::TRegisters &regs, kiv_vfs::CVirtual_File_System &vfs) {
	// TODO
}

void Create_Pipe(kiv_hal::TRegisters &regs, kiv_vfs::CVirtual_File_System &vfs) {
	// TODO
}



void Handle_IO(kiv_hal::TRegisters &regs) {
	//kiv_vfs::CVirtual_File_System &vfs = kiv_vfs::CVirtual_File_System::Get_Instance();

	//switch (static_cast<kiv_os::NOS_File_System>(regs.rax.l)) {
	//	case kiv_os::NOS_File_System::Open_File:		return Open_File(regs, vfs);
	//	case kiv_os::NOS_File_System::Close_Handle:		return Close_File(regs, vfs);
	//	case kiv_os::NOS_File_System::Delete_File:		return Delete_File(regs, vfs);
	//	case kiv_os::NOS_File_System::Write_File:		return Write_File(regs, vfs);
	//	case kiv_os::NOS_File_System::Read_File:		return Read_File(regs, vfs);
	//	case kiv_os::NOS_File_System::Seek:				return Seek(regs, vfs);
	//	case kiv_os::NOS_File_System::Set_Working_Dir:	return Set_Working_Dir(regs, vfs);
	//	case kiv_os::NOS_File_System::Get_Working_Dir:	return Get_Working_Dir(regs, vfs);
	//	case kiv_os::NOS_File_System::Create_Pipe:		return Create_Pipe(regs, vfs);
	//	default:										return; // TODO Handle unknown operation
	//}

	switch (static_cast<kiv_os::NOS_File_System>(regs.rax.l)) {
		case kiv_os::NOS_File_System::Read_File: {
				//viz uvodni komentar u Write_File
				regs.rax.r = Read_Line_From_Console(reinterpret_cast<char*>(regs.rdi.r), regs.rcx.r);
		}
		break;


		case kiv_os::NOS_File_System::Write_File: {
					//Spravne bychom nyni meli pouzit interni struktury kernelu a zadany handle resolvovat na konkretni objekt, ktery pise na konkretni zarizeni/souboru/roury.
					//Ale protoze je tohle jenom kostra, tak to rovnou biosem posleme na konzoli.
					kiv_hal::TRegisters registers;
					registers.rax.h = static_cast<decltype(registers.rax.h)>(kiv_hal::NVGA_BIOS::Write_String);
					registers.rdx.r = regs.rdi.r;
					registers.rcx = regs.rcx;
		
					//preklad parametru dokoncen, zavolame sluzbu
					kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, registers);

					regs.flags.carry |= (registers.rax.r == 0 ? 1 : 0);	//jestli jsme nezapsali zadny znak, tak jiste doslo k nejake chybe
					regs.rax = registers.rcx;	//VGA BIOS nevraci pocet zapsanych znaku, tak predpokladame, ze zapsal vsechny
		}
		break; //Write_File
	}

	/* Nasledujici dve vetve jsou ukazka, ze starsiho zadani, ktere ukazuji, jak mate mapovat Windows HANDLE na kiv_os handle a zpet, vcetne jejich alokace a uvolneni

		case kiv_os::scCreate_File: {
			HANDLE result = CreateFileA((char*)regs.rdx.r, GENERIC_READ | GENERIC_WRITE, (DWORD)regs.rcx.r, 0, OPEN_EXISTING, 0, 0);
			//zde je treba podle Rxc doresit shared_read, shared_write, OPEN_EXISING, etc. podle potreby
			regs.flags.carry = result == INVALID_HANDLE_VALUE;
			if (!regs.flags.carry) regs.rax.x = Convert_Native_Handle(result);
			else regs.rax.r = GetLastError();
		}
									break;	//scCreateFile

		case kiv_os::scClose_Handle: {
				HANDLE hnd = Resolve_kiv_os_Handle(regs.rdx.x);
				regs.flags.carry = !CloseHandle(hnd);
				if (!regs.flags.carry) Remove_Handle(regs.rdx.x);				
					else regs.rax.r = GetLastError();
			}

			break;	//CloseFile

	*/
}