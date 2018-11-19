#include "io.h"
#include "kernel.h"
#include "handles.h"
#include "vfs.h"
#include "process.h"

kiv_vfs::CVirtual_File_System &vfs = kiv_vfs::CVirtual_File_System::Get_Instance();

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

void Set_Result(kiv_hal::TRegisters &regs, kiv_os::NOS_Error result) {
	regs.flags.carry = (result == kiv_os::NOS_Error::Success) ? 0 : 1;
	if (regs.flags.carry) {
		regs.rax.x = static_cast<decltype(regs.rax.x)>(result);
	}
}

kiv_os::NOS_Error Get_Position(kiv_os::THandle vfs_handle, kiv_hal::TRegisters &regs, kiv_vfs::CVirtual_File_System &vfs) {
	try {
		size_t position = vfs.Get_Position(vfs_handle);
		regs.rax.r = static_cast<decltype(regs.rax.r)>(position);
		return kiv_os::NOS_Error::Success;
	}
	catch (...) { // Including TInvalid_Fd_Exception
		return kiv_os::NOS_Error::Unknown_Error;
	}
}

kiv_os::NOS_Error Set_Position(kiv_os::THandle vfs_handle, int position, kiv_os::NFile_Seek seek_offset_type, kiv_vfs::CVirtual_File_System &vfs) {
	try {
		vfs.Set_Position(vfs_handle, position, seek_offset_type);
		return kiv_os::NOS_Error::Success;
	}
	catch (kiv_vfs::TPosition_Out_Of_Range_Exception e) {
		return kiv_os::NOS_Error::Invalid_Argument; // TODO ??
	}
	catch (...) { // Including TInvalid_Fd_Exception
		return kiv_os::NOS_Error::Unknown_Error;
	}
}

kiv_os::NOS_Error Set_Size(kiv_os::THandle vfs_handle, int position, kiv_os::NFile_Seek seek_offset_type, kiv_vfs::CVirtual_File_System &vfs) {
	// TODO
	return kiv_os::NOS_Error::Unknown_Error;
}

void Open_File(kiv_hal::TRegisters &regs) {
	std::string path = reinterpret_cast<char *>(regs.rdx.r);
	kiv_os::NFile_Attributes attributes = static_cast<kiv_os::NFile_Attributes>(regs.rdi.r);
	kiv_os::NOpen_File open_flag = static_cast<kiv_os::NOpen_File>(regs.rcx.r);

	kiv_os::NOS_Error result;
	try {
		kiv_os::THandle handle;
		if (open_flag == kiv_os::NOpen_File::fmOpen_Always) {
			vfs.Open_File(path, attributes, handle);
		}
		else {
			vfs.Create_File(path, attributes, handle);
		}
		handle = kiv_process::CProcess_Manager::Get_Instance().Save_Fd(handle);
		regs.rax.x = static_cast<decltype(regs.rax.x)>(handle);
		result = kiv_os::NOS_Error::Success;
	}
	catch (kiv_vfs::TFd_Table_Full_Exception e) {
		result = kiv_os::NOS_Error::Out_Of_Memory;
	}
	catch (kiv_vfs::TPermission_Denied_Exception e) {
		result = kiv_os::NOS_Error::Permission_Denied;
	}
	catch (kiv_vfs::TFile_Not_Found_Exception e) {
		result = kiv_os::NOS_Error::File_Not_Found;
	}
	catch (kiv_vfs::TNot_Enough_Space_Exception e) { // In case of creating new file
		result = kiv_os::NOS_Error::Not_Enough_Disk_Space;
	}
	catch (...) {
		result = kiv_os::NOS_Error::Unknown_Error;
	}

	Set_Result(regs, result);
}

void Close_File(kiv_hal::TRegisters &regs, kiv_vfs::CVirtual_File_System &vfs) {
	kiv_os::THandle proc_handle = static_cast<kiv_os::THandle>(regs.rdx.x);
	kiv_os::THandle vfs_handle = proc_handle; // TODO
	// TODO proc_handle -> vfs_handle (check if proc_handle is correct)

	kiv_os::NOS_Error result;
	try {
		vfs.Close_File(vfs_handle);
		result = kiv_os::NOS_Error::Success;
	}
	catch (...) { // including TInvalid_Fd_Exception
		result = kiv_os::NOS_Error::Unknown_Error;
	}

	Set_Result(regs, result);
}

void Delete_File(kiv_hal::TRegisters &regs, kiv_vfs::CVirtual_File_System &vfs) {
	std::string path = reinterpret_cast<char *>(regs.rdx.r);

	kiv_os::NOS_Error result;
	try {
		vfs.Delete_File(path);
	}
	catch (kiv_vfs::TFile_Not_Found_Exception e) {
		result = kiv_os::NOS_Error::File_Not_Found;
	}
	catch (kiv_vfs::TPermission_Denied_Exception e) {
		result = kiv_os::NOS_Error::Permission_Denied;
	}
	catch (...) {
		result = kiv_os::NOS_Error::Unknown_Error;
	}

	Set_Result(regs, result);
}

void Write_File(kiv_hal::TRegisters &regs) {
	kiv_os::THandle vfs_handle;
	kiv_os::THandle proc_handle = static_cast<kiv_os::THandle>(regs.rdx.x);
	char *buffer = reinterpret_cast<char *>(regs.rdi.r);
	size_t buf_size = static_cast<size_t>(regs.rcx.r);

	if (!kiv_process::CProcess_Manager::Get_Instance().Get_Fd(proc_handle, vfs_handle)) {
#ifdef _DEBUG
		printf("FD not found\n");
#endif
	}

	kiv_os::NOS_Error result;
	try {
		vfs.Write_File(vfs_handle, buffer, buf_size);
		result = kiv_os::NOS_Error::Success;
	}
	catch (kiv_vfs::TPermission_Denied_Exception e) {
		result = kiv_os::NOS_Error::Permission_Denied;
	}
	catch (...) { // including TInvalid_Fd_Exception
		result = kiv_os::NOS_Error::Unknown_Error;
	}

	Set_Result(regs, result);
}

void Read_File(kiv_hal::TRegisters &regs) {
	kiv_os::THandle vfs_handle;
	kiv_os::THandle proc_handle = static_cast<kiv_os::THandle>(regs.rdx.x);
	char *buffer = reinterpret_cast<char *>(regs.rdi.r);
	size_t buf_size = static_cast<size_t>(regs.rcx.r);
	size_t bytes_read;
	if (!kiv_process::CProcess_Manager::Get_Instance().Get_Fd(proc_handle, vfs_handle)) {

	}

	kiv_os::NOS_Error result;
	try {
		bytes_read = vfs.Read_File(vfs_handle, buffer, buf_size);
		result = kiv_os::NOS_Error::Success;
	}
	catch (kiv_vfs::TPermission_Denied_Exception e) {
		result = kiv_os::NOS_Error::Permission_Denied;
	}
	catch (...) { // including TInvalid_Fd_Exception
		result = kiv_os::NOS_Error::Unknown_Error;
	}
	regs.rax.r = bytes_read;
	Set_Result(regs, result);
}

void Seek(kiv_hal::TRegisters &regs, kiv_vfs::CVirtual_File_System &vfs) {
	kiv_os::THandle proc_handle = static_cast<kiv_os::THandle>(regs.rdx.x);
	kiv_os::NFile_Seek seek_type = static_cast<kiv_os::NFile_Seek>(regs.rcx.l);
	kiv_os::NFile_Seek seek_offset_type = static_cast<kiv_os::NFile_Seek>(regs.rcx.h);
	int position = static_cast<int>(regs.rdi.r);

	kiv_os::THandle vfs_handle = proc_handle; // TODO
	// TODO proc_handle -> vfs_handle (check if proc_handle is correct)

	kiv_os::NOS_Error result;
	switch (seek_type) {
		case kiv_os::NFile_Seek::Get_Position:
			result = Get_Position(vfs_handle, regs, vfs);
			break;

		case kiv_os::NFile_Seek::Set_Position:
			result = Set_Position(vfs_handle, position, seek_offset_type, vfs);
			break;

		case kiv_os::NFile_Seek::Set_Size:
			result = Set_Size(vfs_handle, position, seek_offset_type, vfs);
			break;
	}

	Set_Result(regs, result);
}

void Set_Working_Dir(kiv_hal::TRegisters &regs, kiv_vfs::CVirtual_File_System &vfs) {
	char *path = reinterpret_cast<char *>(regs.rdx.r); // null terminated

	// TODO
}

void Get_Working_Dir(kiv_hal::TRegisters &regs, kiv_vfs::CVirtual_File_System &vfs) {
	char *buffer = reinterpret_cast<char *>(regs.rdx.r);
	size_t buf_size = static_cast<size_t>(regs.rcx.r);

	size_t chars_written = vfs.Get_Working_Directory(buffer, buf_size);

	regs.rax.r = static_cast<size_t>(chars_written);
}

void Create_Pipe(kiv_hal::TRegisters &regs) {
	kiv_os::NOS_Error result;
	kiv_os::THandle *handle_pair = reinterpret_cast<kiv_os::THandle *>(regs.rdx.r);
	kiv_os::THandle in;
	kiv_os::THandle out;

	try {
		vfs.Create_Pipe(in, out);
		result = kiv_os::NOS_Error::Success;
	}
	catch (kiv_vfs::TFd_Table_Full_Exception e) {
		result = kiv_os::NOS_Error::Out_Of_Memory;
	}
	catch (...) {
		result = kiv_os::NOS_Error::Unknown_Error;
	}

	handle_pair[0] = in;
	handle_pair[1] = out;
	
	Set_Result(regs, result);
}


void Handle_IO(kiv_hal::TRegisters &regs) {
	switch (static_cast<kiv_os::NOS_File_System>(regs.rax.l)) {
		case kiv_os::NOS_File_System::Open_File:		
			Open_File(regs);
			break;
	//	case kiv_os::NOS_File_System::Close_Handle:		return Close_File(regs, vfs);
	//	case kiv_os::NOS_File_System::Delete_File:		return Delete_File(regs, vfs);
		case kiv_os::NOS_File_System::Write_File:		return Write_File(regs);
		case kiv_os::NOS_File_System::Read_File:		return Read_File(regs);
	//	case kiv_os::NOS_File_System::Seek:				return Seek(regs, vfs);
	//	case kiv_os::NOS_File_System::Set_Working_Dir:	return Set_Working_Dir(regs, vfs);
	//	case kiv_os::NOS_File_System::Get_Working_Dir:	return Get_Working_Dir(regs, vfs);
		case kiv_os::NOS_File_System::Create_Pipe:		return Create_Pipe(regs);
	//	default:										return; // TODO Handle unknown operation
	}

	//V ostre verzi pochopitelne do switche dejte volani funkci a ne primo vykonny kod
			/*
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
	*/
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