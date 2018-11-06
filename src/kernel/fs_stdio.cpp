#include "fs_stdio.h"



#pragma region Util
namespace tmp {
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
}
#pragma endregion

#pragma region Register
namespace kiv_fs_stdio {
	CFile_System::CFile_System() {

	}

	bool CFile_System::Register() {
		return kiv_vfs::CVirtual_File_System::Get_Instance().Register_File_System(*this);
	}

	kiv_vfs::IMounted_File_System *CFile_System::Create_Mount(const std::string label, const kiv_vfs::TDisk_Number) {
		return new CMount(label);
	}
}
#pragma endregion


#pragma region File
namespace kiv_fs_stdio {
	CFile::CFile(std::shared_ptr<kiv_vfs::TPath> path, kiv_os::NFile_Attributes attributes) {
		mPath = path;
		mAttributes = attributes;
	}

	size_t CFile::Read(char *buffer, size_t buffer_size, int position) {
		return tmp::Read_Line_From_Console(buffer, buffer_size);
	}

	size_t CFile::Write(const char *buffer, size_t buffer_size, int position) {
		kiv_hal::TRegisters registers;
		registers.rax.h = static_cast<decltype(registers.rax.h)>(kiv_hal::NVGA_BIOS::Write_String);
		registers.rdx.r = reinterpret_cast<decltype(registers.rdx.r)>(buffer);
		registers.rcx.r = buffer_size;

		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, registers);

		if (registers.flags.carry == 1) {
			return 0;
		}

		return buffer_size;
	}

	bool CFile::Is_Available_For_Write() {
		return (!(mAttributes == kiv_os::NFile_Attributes::Read_Only));
	}

}
#pragma endregion


#pragma region Mount
namespace kiv_fs_stdio {
	CMount::CMount(std::string label) {
		mLabel = label;
	}
	
	std::shared_ptr<kiv_vfs::IFile> CMount::Open_File(std::shared_ptr<kiv_vfs::TPath> path, kiv_os::NFile_Attributes attributes) {
		std::shared_ptr<kiv_vfs::IFile> file = std::make_shared<CFile>(path, attributes);
		
		return file;
	}	
}

#pragma endregion