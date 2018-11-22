#include "fs_stdio.h"

#include <iostream>

#pragma region Util
namespace tmp { // todo remove tmp
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
	const char *STDIO_NAME = "stdio";

	CFile_System::CFile_System() {
		mName = STDIO_NAME;
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
	CFile::CFile(const kiv_vfs::TPath path, kiv_os::NFile_Attributes attributes) {
		mPath = path;
		mAttributes = attributes;
	}

	size_t CFile::Read(char *buffer, size_t buffer_size, size_t position) {
		return tmp::Read_Line_From_Console(buffer, buffer_size);
	}

	size_t CFile::Write(const char *buffer, size_t buffer_size, size_t position) {
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
		
		/* during mount creating both stdin file and stdout file */
		kiv_vfs::TPath stdin_path;
		stdin_path.absolute_path = "stdio:\\stdin";
		stdin_path.file = "stdin";
		stdin_path.mount = "stdio";
		
		std::shared_ptr<kiv_vfs::IFile> stdin_file = std::make_shared<CFile>(stdin_path, kiv_os::NFile_Attributes::System_File);
		mStdioFiles.insert(std::pair<std::string, std::shared_ptr<kiv_vfs::IFile>>("stdin", stdin_file));

		kiv_vfs::TPath stdout_path;
		stdout_path.absolute_path = "stdio:\\stdout";
		stdout_path.file = "stdout";
		stdout_path.mount = "stdio";

		std::shared_ptr<kiv_vfs::IFile> stdout_file = std::make_shared<CFile>(stdout_path, kiv_os::NFile_Attributes::System_File);
		mStdioFiles.insert(std::pair<std::string, std::shared_ptr<kiv_vfs::IFile>>("stdout", stdout_file));
	}

	std::shared_ptr<kiv_vfs::IFile> CMount::Open_File(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes) {
		if (mStdioFiles.count(path.file)) {
			return mStdioFiles.at(path.file);
		}

		return nullptr;
	}	
}

#pragma endregion