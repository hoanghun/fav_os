#include "fs_fat.h"
#include "../api/api.h"

// TODO Smazat
#include <iostream>
using namespace std;
///////////////

namespace kiv_fs_fat {

#pragma region File
	CFile::CFile(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes, kiv_vfs::TDisk_Number disk_number, TSuperblock &sb) 
		: mDisk_number(disk_number), mSuperblock(sb) 
	{
		mPath = path;
		mAttributes = attributes;
	}

	size_t CFile::Write(const char *buffer, size_t buffer_size, size_t position) {
		// TODO
		return 0;
	}

	size_t CFile::Read(char *buffer, size_t buffer_size, size_t position) {
		// TODO
		return 0;
	}

	bool CFile::Is_Available_For_Write() {
		return (mWrite_count == 0); // TODO correct?
	}
#pragma endregion

#pragma region Mount
	CMount::CMount(std::string label, kiv_vfs::TDisk_Number disk_number) {
		mLabel = label;
		mDisk_Number = disk_number;


		if (!Load_Superblock()) {
			// TODO Handle error
			cout << "FAT - LOAD SUPERBLOCK ERR" << endl;
		}

		// Check if disk is formatted
		if (!Chech_Superblock()) {
			if (!Format_Disk()) {
				// TODO Handle error
				cout << "FAT - FORMAT ERR" << endl;
			}
		}
	}

	std::shared_ptr<kiv_vfs::IFile> CMount::Open_File(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes) {
		// TODO
		return nullptr;
	}
	
	std::shared_ptr<kiv_vfs::IFile> CMount::Create_File(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes) {
		// TODO
		return nullptr;
	}

	bool CMount::Delete_File(const kiv_vfs::TPath &path) {
		// TODO
		return false;
	}

	bool CMount::Load_Superblock() {
		kiv_hal::TRegisters regs;
		kiv_hal::TDisk_Address_Packet dap;

		char buff[FAT_SECTOR_SIZE];

		dap.count = 1;
		dap.lba_index = 0;
		dap.sectors = buff;

		regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);;
		regs.rdx.l = static_cast<decltype(regs.rdx.l)>(mDisk_Number);
		regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&dap);

		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

		mSuperblock = *reinterpret_cast<TSuperblock *>(buff);
		return (regs.flags.carry == 0);
	}

	bool CMount::Chech_Superblock() {
		return (strcmp(FS_NAME, mSuperblock.name) == 0);
	}

	bool CMount::Format_Disk() {
		kiv_hal::TRegisters regs;
		kiv_hal::TDrive_Parameters params;

		regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Drive_Parameters);;
		regs.rdx.l = static_cast<decltype(regs.rdx.l)>(mDisk_Number);
		regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&params);
		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

		// Set up superblock
		strcpy_s(mSuperblock.name, FS_NAME);
		mSuperblock.disk_params = params;
		// ...

		// Write superblock to the first sector
		char *superblock_sector = reinterpret_cast<char *>(&mSuperblock);

		kiv_hal::TDisk_Address_Packet dap;
		dap.count = 1;
		dap.lba_index = 0;
		dap.sectors = superblock_sector;

		regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Write_Sectors);
		regs.rdx.l = static_cast<decltype(regs.rdx.l)>(mDisk_Number);
		regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&dap);

		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

		return (regs.flags.carry == 0);
	}

	void Read_Tmp() {
		kiv_hal::TRegisters regs;
		kiv_hal::TDisk_Address_Packet dap;

		char a[5];

		dap.count = 1;
		dap.lba_index = 0;
		dap.sectors = a;

		regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);;
		regs.rdx.l = static_cast<decltype(regs.rdx.l)>(0x81);
		regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&dap); // TDisk_Address_Packet address
		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

		if (regs.flags.carry)
			cout << "CHYBA PRI CTENI" << endl;
		else
			cout << "CTENI OK" << endl;

		cout << a << endl;
	}

	void Write_Tmp() {
		kiv_hal::TRegisters regs;
		kiv_hal::TDisk_Address_Packet dap;

		char *sectors = "AHOJ";

		dap.count = 1;
		dap.lba_index = 0;
		dap.sectors = sectors;

		regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Write_Sectors);;
		regs.rdx.l = static_cast<decltype(regs.rdx.l)>(0x81);
		regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&dap); // TDisk_Address_Packet address
		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

		if (regs.flags.carry)
			cout << "CHYBA PRI ZAPISU" << endl;
		else
			cout << "ZAPIS OK" << endl;
	}
#pragma endregion

#pragma region Filesystem
	CFile_System::CFile_System() {
		mName = FS_NAME;
	}

	bool CFile_System::Register() {
		return kiv_vfs::CVirtual_File_System::Get_Instance().Register_File_System(*this);
	}

	kiv_vfs::IMounted_File_System *CFile_System::Create_Mount(const std::string label, const kiv_vfs::TDisk_Number disk_number) {
		return new CMount(label, disk_number);
	}
#pragma endregion
}