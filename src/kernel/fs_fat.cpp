#include "fs_fat.h"
#include "../api/api.h"

// TODO Smazat
#include <iostream>
#include <string.h>
using namespace std;
///////////////

namespace kiv_fs_fat {
	// FAT entry status
	const uint32_t FAT_FREE = static_cast<uint32_t>(-2);
	const uint32_t FAT_RESERVED = static_cast<uint32_t>(-3);
	const uint32_t FAT_EOF = static_cast<uint32_t>(-4);

	const size_t MAX_DIR_ENTRIES = 21;
	const char *FAT_NAME = "fat";
	const TFAT_Dir_Entry root_dir_entry{ "\\" };


#pragma region IO Utils
	CFAT_Utils::CFAT_Utils(TSuperblock &sb, kiv_vfs::TDisk_Number disk_number)
		: mSb(sb), mDisk_number(disk_number)
	{
	}

	CFAT_Utils::CFAT_Utils(kiv_vfs::TDisk_Number disk_number)
		: mSb(TSuperblock{}), mDisk_number(disk_number)
	{
	}


	bool CFAT_Utils::Write_To_Disk(char *sectors, uint64_t first_sector, uint64_t num_of_sectors) {
		kiv_hal::TRegisters regs;
		kiv_hal::TDisk_Address_Packet dap;

		dap.lba_index = first_sector;
		dap.count = num_of_sectors;
		dap.sectors = sectors;

		regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Write_Sectors);;
		regs.rdx.l = static_cast<decltype(regs.rdx.l)>(mDisk_number);
		regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&dap);

		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

		return (regs.flags.carry == 0);
	}

	bool CFAT_Utils::Read_From_Disk(char *buffer, uint64_t first_sector, uint64_t num_of_sectors) {
		kiv_hal::TRegisters regs;
		kiv_hal::TDisk_Address_Packet dap;

		dap.lba_index = first_sector;
		dap.count = num_of_sectors;
		dap.sectors = buffer;

		regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);;
		regs.rdx.l = static_cast<decltype(regs.rdx.l)>(mDisk_number);
		regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&dap);

		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

		return (regs.flags.carry == 0);
	}

	bool CFAT_Utils::Write_Clusters(char *clusters, uint64_t first_cluster, uint64_t num_of_clusters) {
		return Write_To_Disk(clusters, first_cluster * mSb.sectors_per_cluster, num_of_clusters * mSb.sectors_per_cluster);
	}

	bool CFAT_Utils::Read_Clusters(char *buffer, uint64_t first_cluster, uint64_t num_of_clusters) {
		return Read_From_Disk(buffer, first_cluster * mSb.sectors_per_cluster, num_of_clusters * mSb.sectors_per_cluster);
	}

	bool CFAT_Utils::Write_Data_Cluster(char *clusters, TFAT_Entry fat_entry) {
		return Write_Clusters(clusters, mSb.data_first_cluster + fat_entry, 1);
	}

	bool CFAT_Utils::Read_Data_Cluster(char *buffer, TFAT_Entry fat_entry) {
		return Read_Clusters(buffer, mSb.data_first_cluster + fat_entry, 1);
	}

	bool CFAT_Utils::Set_Fat_Entries_Value(std::vector<TFAT_Entry> &entries, TFAT_Entry value) {
		std::map<TFAT_Entry, TFAT_Entry> map;
		for (auto it = entries.begin(); it != entries.end(); ++it) {
			map.insert(std::pair<TFAT_Entry, TFAT_Entry>(*it, value));
		}

		if (!Write_Fat_Entries(map)) {
			return false;
		}

		return true;
	}

	bool CFAT_Utils::Get_Free_Fat_Entries(std::vector<TFAT_Entry> &entries, size_t number_of_entries) {
		size_t cluster_size = mSb.sectors_per_cluster * mSb.disk_params.bytes_per_sector;
		size_t entries_per_cluster = cluster_size / sizeof(TFAT_Entry);
		
		char *cluster_buffer = new char[cluster_size];

		size_t curr_cluster = mSb.fat_table_first_cluster;
		TFAT_Entry curr_entry = 0;

		TFAT_Entry entry;
		while (curr_entry < mSb.fat_table_number_of_entries) {

			// Read new cluster if needed
			if (curr_entry % entries_per_cluster == 0) {
				Read_Clusters(cluster_buffer, curr_cluster, 1);
				curr_cluster++;
			}

			// Load current FAT entry
			memcpy(&entry, cluster_buffer + (curr_entry % entries_per_cluster) * sizeof(TFAT_Entry), sizeof(TFAT_Entry));

			// Free entry found
			if (entry == FAT_FREE) {
				entries.push_back(curr_entry);

				// All requested entries found
				if (entries.size() == number_of_entries) {
					return true;
				}

			}

			curr_entry++;
		}

		return Set_Fat_Entries_Value(entries, FAT_RESERVED);
	}

	bool CFAT_Utils::Write_Fat_Entries(std::map<TFAT_Entry, TFAT_Entry> &entries) {
		size_t cluster_size = mSb.sectors_per_cluster * mSb.disk_params.bytes_per_sector;
		char *cluster_buffer = new char[cluster_size];

		size_t cluster_loaded = static_cast<size_t>(-1);
		size_t cluster_needed;

		size_t order, order_in_cluster;
		TFAT_Entry value;
		for (auto it = entries.begin(); it != entries.end(); it++) {
			order = it->first;
			value = it->second;

			cluster_needed = (order / mSb.fat_table_number_of_entries) + mSb.fat_table_first_cluster;

			// This FAT entry is not located in currently loaded cluster
			if (cluster_needed != cluster_loaded) {

				// Store current cluster
				if (cluster_loaded != static_cast<size_t>(-1)) {
					if (!Write_Clusters(cluster_buffer, cluster_needed, 1)) {
						delete[] cluster_buffer;
						return false;
					}
				}

				// Load needed cluster
				if (!Read_Clusters(cluster_buffer, cluster_needed, 1)) {
					delete[] cluster_buffer;
					return false;
				}

				cluster_loaded = cluster_needed;
			}

			order_in_cluster = order % mSb.fat_table_number_of_entries;

			// Store new FAT entry into a buffer
			memcpy(cluster_buffer + order_in_cluster * sizeof(TFAT_Entry), &value, sizeof(TFAT_Entry));
		}

		// Store current cluster
		if (cluster_loaded != static_cast<size_t>(-1)) {
			if (!Write_Clusters(cluster_buffer, cluster_needed, 1)) {
				delete[] cluster_buffer;
				return false;
			}
		}

		delete[] cluster_buffer;
		return true;
	}

	bool CFAT_Utils::Get_File_Fat_Entries(TFAT_Entry first_entry, std::vector<TFAT_Entry> &entries) {
		size_t cluster_size = mSb.sectors_per_cluster * mSb.disk_params.bytes_per_sector;
		char *cluster_buffer = new char[cluster_size];

		size_t cluster_loaded = static_cast<size_t>(-1);
		size_t cluster_needed;

		size_t order_in_cluster;

		TFAT_Entry value = first_entry;
		while (value != FAT_EOF) {
			entries.push_back(value);

			cluster_needed = (value / mSb.fat_table_number_of_entries) + mSb.fat_table_first_cluster;

			// FAT entry is not located in currently loaded cluster -> Load needed cluster
			if (cluster_needed != cluster_loaded) {
				if (!Read_Clusters(cluster_buffer, cluster_needed, 1)) {
					delete[] cluster_buffer;
					return false;
				}
				cluster_loaded = cluster_needed;
			}

			order_in_cluster = value % mSb.fat_table_number_of_entries;

			// Load value
			memcpy(&value, cluster_buffer + order_in_cluster * sizeof(TFAT_Entry), sizeof(TFAT_Entry));
		}

		delete[] cluster_buffer;

		return true;
	}

	bool CFAT_Utils::Free_File_Fat_Entries(TFAT_Dir_Entry &entry) {
		std::vector<TFAT_Entry> entries;

		if (!Get_File_Fat_Entries(entry.start, entries)) {
			return false;
		}

		return Set_Fat_Entries_Value(entries, FAT_FREE);;
	}

	bool CFAT_Utils::Load_Directory(std::vector<TFAT_Dir_Entry> dirs_from_root, std::shared_ptr<IDirectory> &directory) {
		TFAT_Dir_Entry dir_entry = dirs_from_root.back();

		// Dir is root
		if (strcmp(dir_entry.name, root_dir_entry.name) == 0) {
			directory = make_shared<CRoot>(this);
		}
		else {
			kiv_vfs::TPath path;
			path.file = dir_entry.name;
			dirs_from_root.pop_back();
			directory = make_shared<CDirectory>(path, dir_entry, dirs_from_root, this);
		}

		return true;
	}

	void CFAT_Utils::Set_Superblock(TSuperblock sb) {
		mSb = sb;
	}

	TSuperblock &CFAT_Utils::Get_Superblock() {
		return mSb;
	}

#pragma endregion

#pragma region Abstract directory

	IDirectory::IDirectory(CFAT_Utils *utils) 
		: mUtils(utils)
	{
	}

	size_t IDirectory::Read(char *buffer, size_t buffer_size, size_t position) {
		if (!Load()) {
			return 0;
		}
		// Buffer is not big enough even for one entry
		if (buffer_size < sizeof(kiv_os::TDir_Entry)) {
			return 0;
		}

		kiv_os::TDir_Entry os_dir_entry;
		TFAT_Dir_Entry fat_dir_entry;

		size_t read_count = 0;
		size_t fat_entry_index;
		for (size_t i = 0; i < buffer_size; i += sizeof(kiv_os::TDir_Entry)) {
			fat_entry_index = (i + position) / sizeof(kiv_os::TDir_Entry);

			// All entries have been read
			if (fat_entry_index >= mEntries.size()) {
				break;
			}

			fat_dir_entry = mEntries.at(fat_entry_index);

			// Fat dir entry -> os dir entry
			strcpy_s(os_dir_entry.file_name, fat_dir_entry.name);
			os_dir_entry.file_attributes = static_cast<decltype(os_dir_entry.file_attributes)>(fat_dir_entry.attributes);

			// Store direntry into a buffer
			memcpy(buffer + i, &os_dir_entry, sizeof(kiv_os::TDir_Entry));

			read_count += sizeof(kiv_os::TDir_Entry);
		}

		return read_count;
	}

	bool IDirectory::Is_Empty() {
		if (!Load()) {
			return false;
		}

		return (mEntries.size() == 0);
	}

	std::shared_ptr<kiv_vfs::IFile> IDirectory::Create_File(const kiv_vfs::TPath path, kiv_os::NFile_Attributes attributes) {
		if (!Load()) {
			return false;
		}

		// Directory is full
		if (mEntries.size() == MAX_DIR_ENTRIES) {
			return nullptr;
		}

		std::vector<TFAT_Entry> entry;
		if (!mUtils->Get_Free_Fat_Entries(entry, 1)) {
			return nullptr;
		}

		// Create directory entry
		TFAT_Dir_Entry dir_entry;

		dir_entry.attributes = attributes;
		dir_entry.filesize = 0;
		strcpy_s(dir_entry.name, path.file.c_str()); // TODO unsafe maybe? (c_str() appends '\0')
		dir_entry.start = entry[0];

		mEntries.push_back(dir_entry);

		// Write directory entry to disk
		std::map<TFAT_Entry, TFAT_Entry> entry_map;
		entry_map.insert(std::pair<TFAT_Entry, TFAT_Entry>(entry[0], FAT_EOF));

		if (!mUtils->Write_Fat_Entries(entry_map)) {
			mUtils->Set_Fat_Entries_Value(entry, FAT_FREE);
			return nullptr;
		}

		mSize += sizeof(TFAT_Dir_Entry);

		if (!Save()) {
			mUtils->Set_Fat_Entries_Value(entry, FAT_FREE);
			return nullptr;
		}

		auto result = Make_File(path, dir_entry);
		if (!result) {
			mUtils->Set_Fat_Entries_Value(entry, FAT_FREE);
		}

		return result;
	}

	bool IDirectory::Remove_File(const kiv_vfs::TPath &path) {
		if (!Load()) {
			return false;
		}

		for (auto it = mEntries.begin(); it != mEntries.end(); ++it) {
			// File found
			if (it->name == path.file) {

				if (!mUtils->Free_File_Fat_Entries(*it)) {
					return false;
				}

				// This is the only dir entry -> just remove it
				if (mEntries.size() == 1) {
					mEntries.erase(it);
				}
				// Replace this entry with last one
				else {
					auto last_entry = mEntries.back();
					*it = last_entry;
					mEntries.pop_back();
				}

				mSize -= sizeof(TFAT_Dir_Entry);

				if (!Save()) {
					return false;
				}
				return true;
			}
		}

		return false;
	}

	bool IDirectory::Find(std::string filename, TFAT_Dir_Entry &entry) {
		if (!Load()) {
			return false;
		}
		for (auto it = mEntries.begin(); it != mEntries.end(); ++it) {
			if (it->name == filename) {
				entry = *it;
				return true;
			}
		}

		return false;
	}

	bool IDirectory::Change_Entry_Size(std::string filename, uint32_t filesize) {
		if (!Load()) {
			return false;
		}

		for (auto it = mEntries.begin(); it != mEntries.end(); ++it) {
			if (it->name == filename) {
				it->filesize = filesize;
				if (!Save()) {
					return false;
				}
				return true;
			}
		}

		return false;
	}

	bool IDirectory::Get_Entry_Size(std::string filename, uint32_t &filesize) {
		if (!Load()) {
			return false;
		}

		for (auto it = mEntries.begin(); it != mEntries.end(); ++it) {
			if (it->name == filename) {
				filesize = it->filesize;
				return true;
			}
		}

		return false;
	}
#pragma endregion

#pragma region Subdirectory
	CDirectory::CDirectory(const kiv_vfs::TPath path, TFAT_Dir_Entry &dir_entry, std::vector<TFAT_Dir_Entry> dirs_to_parent, CFAT_Utils *utils)
		: IDirectory(utils), mDir_entry(dir_entry), mDirs_to_parent(dirs_to_parent)
	{
		mPath = path;
		mAttributes = dir_entry.attributes;
		mSize = dir_entry.filesize;
	}

	CDirectory::CDirectory(TFAT_Dir_Entry &dir_entry, CFAT_Utils *utils)
		: IDirectory(utils)
	{
		kiv_vfs::TPath path;
		CDirectory(path, dir_entry, std::vector<TFAT_Dir_Entry>{}, utils);
	}

	std::shared_ptr<kiv_vfs::IFile> CDirectory::Make_File(kiv_vfs::TPath path, TFAT_Dir_Entry entry) {
		std::vector<TFAT_Dir_Entry> dirs_to_this = { mDirs_to_parent };
		dirs_to_this.push_back(mDir_entry);

		if (entry.attributes == kiv_os::NFile_Attributes::Directory) {
			return std::make_shared<CDirectory>(path, entry, dirs_to_this, mUtils);
		}
		else {
			return std::make_shared<CFile>(path, entry, dirs_to_this, mUtils);
		}
	}

	bool CDirectory::Load() {
		mEntries.clear();

		// Load size
		std::shared_ptr<IDirectory> parent;
		if (!mUtils->Load_Directory(mDirs_to_parent, parent)) {
			return false;
		}
		parent->Get_Entry_Size(mPath.file, mSize);

		// Read data from disk
		char *buffer = new char[mUtils->Get_Superblock().sectors_per_cluster * mUtils->Get_Superblock().disk_params.bytes_per_sector];
		if (!mUtils->Read_Data_Cluster(buffer, mDir_entry.start)) {
			delete[] buffer;
			return false;
		}

		// Parse entries
		TFAT_Dir_Entry entry;
		size_t number_of_entries = mSize / sizeof(TFAT_Dir_Entry);

		for (size_t i = 0; i < number_of_entries; i++) {
			memcpy(&entry, buffer + i * sizeof(TFAT_Dir_Entry), sizeof(TFAT_Dir_Entry));
			mEntries.push_back(entry);
		}

		delete[] buffer;

		return true;
	}

	bool CDirectory::Save() {
		char *buffer = new char[mUtils->Get_Superblock().sectors_per_cluster * mUtils->Get_Superblock().disk_params.bytes_per_sector];

		// Save entries
		size_t address = 0;
		for (auto it = mEntries.begin(); it != mEntries.end(); ++it) {
			memcpy(buffer + address, &(*it), sizeof(TFAT_Dir_Entry));
			address += sizeof(TFAT_Dir_Entry);
		}

		TFAT_Dir_Entry entry;
		for (int i = 0; i < mEntries.size(); i++) {
			memcpy(&entry, buffer + i * sizeof(TFAT_Dir_Entry), sizeof(TFAT_Dir_Entry));
		}

		bool res = mUtils->Write_Data_Cluster(buffer, mDir_entry.start);
		delete[] buffer;

		// Save size of directory
		std::shared_ptr<IDirectory> parent;
		if (!mUtils->Load_Directory(mDirs_to_parent, parent)) {
			return nullptr;
		}
		parent->Change_Entry_Size(mPath.file, static_cast<uint32_t>(mEntries.size() * sizeof(TFAT_Dir_Entry)));

		return res;
	}

#pragma endregion

#pragma region Root
	CRoot::CRoot(CFAT_Utils *utils)
		: IDirectory(utils)
	{
		mAttributes = kiv_os::NFile_Attributes::Directory;
	}

	std::shared_ptr<kiv_vfs::IFile> CRoot::Make_File(kiv_vfs::TPath path, TFAT_Dir_Entry entry) {
		std::vector<TFAT_Dir_Entry> dirs_to_this = { root_dir_entry };

		if (entry.attributes == kiv_os::NFile_Attributes::Directory) {
			return std::make_shared<CDirectory>(path, entry, dirs_to_this, mUtils);
		}
		else {
			return std::make_shared<CFile>(path, entry, dirs_to_this, mUtils);
		}
	}

	bool CRoot::Load() {
		mEntries.clear();
		char *buffer = new char[mUtils->Get_Superblock().sectors_per_cluster * mUtils->Get_Superblock().disk_params.bytes_per_sector];
		if (!mUtils->Read_Clusters(buffer, mUtils->Get_Superblock().root_cluster, 1)) {
			delete[] buffer;
			return false;
		}

		// Parse size of root
		memcpy(&mSize, buffer, sizeof(mSize));

		// Parse content of root
		TFAT_Dir_Entry entry;
		size_t number_of_entries = mSize / sizeof(TFAT_Dir_Entry);

		for (size_t i = 0; i < number_of_entries; i++) {
			memcpy(&entry, buffer + sizeof(mSize) + i * sizeof(TFAT_Dir_Entry), sizeof(TFAT_Dir_Entry));
			mEntries.push_back(entry);
		}

		delete[] buffer;
		return true;
	}

	bool CRoot::Save() {
		char *buffer = new char[mUtils->Get_Superblock().sectors_per_cluster * mUtils->Get_Superblock().disk_params.bytes_per_sector];

		// Save size of root
		memcpy(buffer, &mSize, sizeof(mSize));

		// Save entries
		size_t address = sizeof(mSize);
		for (auto it = mEntries.begin(); it != mEntries.end(); ++it) {
			memcpy(buffer + address, &(*it), sizeof(TFAT_Dir_Entry));
			address += sizeof(TFAT_Dir_Entry);
		}

		bool res = mUtils->Write_Clusters(buffer, mUtils->Get_Superblock().root_cluster, 1);

		delete[] buffer;

		return res;
	}
#pragma endregion

#pragma region File
	CFile::CFile(const kiv_vfs::TPath path, TFAT_Dir_Entry &dir_entry, std::vector<TFAT_Dir_Entry> dirs_to_parent, CFAT_Utils *utils)
		: mUtils(utils), mDirs_to_parent(dirs_to_parent)
	{
		mPath = path;
		mAttributes = dir_entry.attributes;
		mSize = dir_entry.filesize;
		if (!mUtils->Get_File_Fat_Entries(dir_entry.start, mFat_entries)) {
			// TODO Handle error
		}
	}

	size_t CFile::Write(const char *buffer, size_t buffer_size, size_t position) {
		if (buffer_size == 0) {
			return 0;
		}

		// Get number of bytes to write (whole buffer or bytes before '\0')
		size_t bytes_to_write = buffer_size;
		for (int i = 0; i < buffer_size; i++) {
			if (buffer[i] == '\0') {
				bytes_to_write = i + 1;
				break;
			}
		}

		size_t cluster_size = mUtils->Get_Superblock().sectors_per_cluster * mUtils->Get_Superblock().disk_params.bytes_per_sector;
		size_t first_cluster = position / cluster_size;
		size_t last_cluster = (position + bytes_to_write) / cluster_size;
		size_t clusters_needed = last_cluster + 1;

		// Need new clusters
		std::vector<TFAT_Entry> new_entries;
		if (mFat_entries.size() < clusters_needed) {
			// Get free entries
			size_t num_of_new_entries = clusters_needed - mFat_entries.size();
			if (!mUtils->Get_Free_Fat_Entries(new_entries, num_of_new_entries)) {
				return 0;
			}

			// Write new entries
			std::map<TFAT_Entry, TFAT_Entry> entry_map;
			entry_map.insert(std::pair<TFAT_Entry, TFAT_Entry>(mFat_entries.back(), new_entries.at(0)));
			for (size_t i = 1; i < new_entries.size(); i++) {
				entry_map.insert(std::pair<TFAT_Entry, TFAT_Entry>(new_entries.at(i - 1), new_entries.at(i)));
			}
			if (!mUtils->Write_Fat_Entries(entry_map)) {
				mUtils->Set_Fat_Entries_Value(new_entries, FAT_FREE);
				return 0;
			}

			// Add new entries to a vector of current entries
			mFat_entries.insert(mFat_entries.end(), new_entries.begin(), new_entries.end());
		}

		// Write to clusters
		char *cluster = new char[cluster_size];
		size_t bytes_to_write_in_cluster;
		size_t bytes_written = 0;
		for (size_t i = first_cluster; i <= last_cluster; i++) {
			if (!mUtils->Read_Data_Cluster(cluster, mFat_entries.at(i))) {
				delete[] cluster;
				return 0;
			}

			// The position has to be taken into consideration in the first cluster
			if (i == first_cluster) {
				bytes_to_write_in_cluster = (bytes_to_write > (cluster_size - position))
					? (cluster_size - position)
					: bytes_to_write;
				memcpy(cluster + position, buffer, bytes_to_write_in_cluster);
			}
			else {
				bytes_to_write_in_cluster = ((bytes_to_write - bytes_written) > cluster_size)
					? cluster_size
					: (bytes_to_write - bytes_written);
				memcpy(cluster, buffer + bytes_written, bytes_to_write_in_cluster);
			}

			if (!mUtils->Write_Data_Cluster(cluster, mFat_entries.at(i))) {
				delete[] cluster;
				return 0;
			}
			bytes_written += bytes_to_write_in_cluster;
		}

		// Change filesize if needed
		if (position + bytes_to_write > mSize) {
			mSize = static_cast<uint32_t>(position + bytes_to_write);
			std::shared_ptr<IDirectory> parent;
			if (!mUtils->Load_Directory(mDirs_to_parent, parent)) {
				return 0;
			}
			parent->Change_Entry_Size(mPath.file, mSize);
		}

		delete[] cluster;
		return bytes_written;
	}

	size_t CFile::Read(char *buffer, size_t buffer_size, size_t position) {
		if (buffer_size == 0) {
			return 0;
		}

		// Get number of bytes to read (whole buffer or rest of the file)
		size_t bytes_to_read = (position + buffer_size < mSize) 
			? buffer_size 
			: (mSize - position);

		size_t cluster_size = mUtils->Get_Superblock().sectors_per_cluster * mUtils->Get_Superblock().disk_params.bytes_per_sector;
		size_t first_cluster = position / cluster_size;
		size_t last_cluster = (position + bytes_to_read) / cluster_size;

		// Read from clusters
		char *cluster = new char[cluster_size];
		size_t bytes_to_read_in_cluster;
		size_t bytes_read = 0;
		for (size_t i = first_cluster; i <= last_cluster; i++) {
			if (!mUtils->Read_Data_Cluster(cluster, mFat_entries.at(i))) {
				delete[] cluster;
				return 0;
			}

			// The position has to be taken into consideration in the first cluster
			if (i == first_cluster) {
				bytes_to_read_in_cluster = (bytes_to_read > (cluster_size - position))
					? (cluster_size - position)
					: (bytes_to_read - bytes_read);
				memcpy(buffer, cluster + position, bytes_to_read_in_cluster);
			} 
			else {
				bytes_to_read_in_cluster = ((bytes_to_read - bytes_read) > cluster_size)
					? cluster_size
					: (bytes_to_read - bytes_read);

				memcpy(buffer + bytes_read, cluster, bytes_to_read_in_cluster);
			}
			bytes_read += bytes_to_read_in_cluster;
		}

		delete[] cluster;
		return bytes_read;
	}

	bool CFile::Is_Available_For_Write() {
		return (mWrite_count == 0); // TODO correct?
	}

	size_t CFile::Get_Size() {
		return mSize;
	}
#pragma endregion

#pragma region Mount
	CMount::CMount(std::string label, kiv_vfs::TDisk_Number disk_number) {
		mLabel = label;
		mDisk_Number = disk_number;
		mUtils = new CFAT_Utils(disk_number);

		kiv_hal::TDrive_Parameters disk_params;
		if (!Load_Disk_Params(disk_params)) {
			cout << "FAT - Couldn't load disk params" << endl;
			return;
			// TODO Handle error
		}

		if (!Load_Superblock(disk_params)) { 
			cout << "FAT - LOAD SUPERBLOCK ERR" << endl;
			return;
			// TODO Handle error
		}

		// Check if disk is formatted
		if (!Chech_Superblock()) { 
			cout << "FAT - NOT FORMATTED" << endl;
			if (!Format_Disk(disk_params)) { 
				cout << "FAT - FORMAT ERR" << endl;
				return;
				// TODO Handle error
			}
		}

		mUtils->Set_Superblock(mSuperblock);
		root = std::make_shared<CRoot>(mUtils); // TODO Handle error
	}

	CMount::~CMount() {
		delete mUtils;
	}

	std::shared_ptr<kiv_vfs::IFile> CMount::Open_File(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes) {
		std::vector<TFAT_Dir_Entry> entries_from_root{ root_dir_entry };
		TFAT_Dir_Entry entry;
		std::shared_ptr<IDirectory> directory;

		// Open root
		if (path.file.length() == 0) {
			return root;
		}

		// File is directly in the root
		if (path.path.empty()) {
			if (!root->Find(path.file, entry)) {
				return nullptr;
			}
			return root->Make_File(path, entry);
		}

		// Find parent
		for (int i = 0; i < path.path.size(); i++) {
			mUtils->Load_Directory(entries_from_root, directory);
			// Directory does not exist
			if (!directory->Find(path.path[i], entry)) {
				return nullptr;
			}
			entries_from_root.push_back(entry);
		}

		mUtils->Load_Directory(entries_from_root, directory);

		// File does not exists
		if (!directory->Find(path.file, entry)) {
			return nullptr;
		}

		return directory->Make_File(path, entry);
	}
	
	std::shared_ptr<kiv_vfs::IFile> CMount::Create_File(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes) {

		// TODO check filename?
		if (path.file.length() == 0) {
			return nullptr;
		}

		// Create file directly in the root
		if (path.path.empty()) {
			if (root->Find(path.file, TFAT_Dir_Entry{})) {
				Delete_File(path);
			}
			return root->Create_File(path, attributes);
		}

		std::vector<TFAT_Dir_Entry> entries_from_root { root_dir_entry };
		TFAT_Dir_Entry entry;
		std::shared_ptr<IDirectory> directory;
		kiv_vfs::TPath tmp_path;

		// Find parent
		for (int i = 0; i < path.path.size(); i++) {
			mUtils->Load_Directory(entries_from_root, directory);
			// Directory not exists -> Create it
			if (!directory->Find(path.path[i], entry)) {
				tmp_path.file = path.path[i];
				if (!directory->Create_File(tmp_path, kiv_os::NFile_Attributes::Directory)) {
					return nullptr;
				}
				directory->Find(path.path[i], entry);
			}

			tmp_path.file = path.path[i];
			entries_from_root.push_back(entry);
		}

		// File already exists -> Remove it
		mUtils->Load_Directory(entries_from_root, directory);
		if (directory->Find(path.file, entry)) {
			Delete_File(path);
		}
		return directory->Create_File(path, attributes);
	}

	bool CMount::Delete_File(const kiv_vfs::TPath &path) {
		kiv_vfs::TPath parent_path;

		// Get path of file's parent
		parent_path.mount = path.mount;
		parent_path.path = path.path;

		if (parent_path.path.size() != 0) {
			// File is not in the root
			parent_path.file = parent_path.path.back();
			parent_path.path.pop_back();
		}

		// Open file's parent
		auto parent_file = Open_File(parent_path, (kiv_os::NFile_Attributes)(0));
		if (!parent_file || !parent_file->Is_Directory()) {
			return false;
		}

		// Remove file from the parent
		std::shared_ptr<IDirectory> parent_dir = std::dynamic_pointer_cast<IDirectory>(parent_file);
		return parent_dir->Remove_File(path);
	}

	bool CMount::Load_Superblock(kiv_hal::TDrive_Parameters &params) {
		char *buff = new char[params.bytes_per_sector];

		bool result = mUtils->Read_From_Disk(buff, 0, 1);
		if (result) {
			mSuperblock = *reinterpret_cast<TSuperblock *>(buff);
		}

		delete[] buff;

		return result;
	}

	bool CMount::Chech_Superblock() {
		return (strcmp(FAT_NAME, mSuperblock.name) == 0);
	}

	bool CMount::Format_Disk(kiv_hal::TDrive_Parameters &params) {
		cout << "FAT - Formating" << endl;

		size_t sectors_per_cluster = 1;
		size_t cluster_size = sectors_per_cluster * params.bytes_per_sector;
		size_t disk_size = params.absolute_number_of_sectors * params.bytes_per_sector;
		size_t available_space = disk_size - (2 * cluster_size); // Disk size - superblock cluster - root cluster

		// TODO
		// Set up superblock
		//strcpy_s(mSuperblock.name, FAT_NAME);
		//mSuperblock.disk_params = params;
		//mSuperblock.fat_table_first_cluster = 1;
		//mSuperblock.sectors_per_cluster = sectors_per_cluster;
		//mSuperblock.fat_table_number_of_entries = available_space / (sizeof(TFAT_Dir_Entry) + cluster_size);
		//mSuperblock.root_cluster = 1 + mSuperblock.fat_table_number_of_entries;
		//mSuperblock.data_first_cluster = mSuperblock.root_cluster + 1; 

		strcpy_s(mSuperblock.name, FAT_NAME);
		mSuperblock.disk_params = params;
		mSuperblock.fat_table_first_cluster = 1;
		mSuperblock.sectors_per_cluster = 1;
		mSuperblock.fat_table_number_of_entries = 10;
		mSuperblock.root_cluster = 2;
		mSuperblock.data_first_cluster = 3;

		// Write superblock to the first sector
		char *superblock_sector = reinterpret_cast<char *>(&mSuperblock);
		if (!mUtils->Write_To_Disk(superblock_sector, 0, 1)) {
			return false;
		}

		// Init FAT table
		if (!Init_Fat_Table()) {
			cout << "FAT - INIT FAT TABLE ERROR" << endl;
			return false;
		}

		if (!Init_Root()) {
			cout << "FAT - INIT ROOT" << endl;
			return false;
		}

		cout << "FAT - Formating DONE" << endl;

		return true;
	}

	bool CMount::Load_Disk_Params(kiv_hal::TDrive_Parameters &params) {
		kiv_hal::TRegisters regs;

		regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Drive_Parameters);;
		regs.rdx.l = static_cast<decltype(regs.rdx.l)>(mDisk_Number);
		regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&params);
		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

		return (regs.flags.carry == 0);
	}

	bool CMount::Init_Fat_Table() {
		size_t entry_size = sizeof(TFAT_Entry);
		size_t cluster_size = mSuperblock.sectors_per_cluster * mSuperblock.disk_params.bytes_per_sector;
		size_t entries_per_cluster = cluster_size / entry_size;
		size_t clusters_needed = ((mSuperblock.fat_table_number_of_entries % entries_per_cluster) == 0)
			? (mSuperblock.fat_table_number_of_entries / entries_per_cluster)
			: ((mSuperblock.fat_table_number_of_entries / entries_per_cluster) + 1);

		char *buffer = new char[clusters_needed * cluster_size];

		// Init FAT entries (ensure that one entry isn't spreaded over two clusters)
		TFAT_Entry entry = FAT_FREE;
		char *free_entry_casted = reinterpret_cast<char *>(&entry);

		size_t curr_cluster = 0;
		size_t cluster_address, cluster_offset, address;

		for (size_t i = 0; i < mSuperblock.fat_table_number_of_entries; i++) {
			if ((i / entries_per_cluster != 0) && (i % entries_per_cluster == 0)) {
				curr_cluster++;
			}

			cluster_address = curr_cluster * cluster_size;
			cluster_offset = ((i * entry_size) - (curr_cluster * cluster_size));

			address = cluster_address + cluster_offset;
			memcpy(buffer + address, free_entry_casted, entry_size);
		}

		// Write FAT table
		bool write_result = mUtils->Write_Clusters(buffer, mSuperblock.fat_table_first_cluster, clusters_needed);

		delete[] buffer;

		return write_result;
	}

	bool CMount::Init_Root() {
		char *buffer = new char[mSuperblock.sectors_per_cluster * mSuperblock.disk_params.bytes_per_sector];

		uint32_t size = 0;
		memcpy(buffer, &size, sizeof(size));

		bool result = mUtils->Write_Clusters(buffer, mSuperblock.root_cluster, 1);

		delete[] buffer;
		return result;
	}

#pragma endregion

#pragma region Filesystem
	CFile_System::CFile_System() {
		mName = FAT_NAME;
	}

	bool CFile_System::Register() {
		return kiv_vfs::CVirtual_File_System::Get_Instance().Register_File_System(*this);
	}

	kiv_vfs::IMounted_File_System *CFile_System::Create_Mount(const std::string label, const kiv_vfs::TDisk_Number disk_number) {
		return new CMount(label, disk_number);
	}
#pragma endregion
}