#pragma once
#include <mutex>
#include <map>

#include "vfs.h"
#include "../api/api.h"

namespace kiv_fs_fat {

	struct TSuperblock;
	struct TFAT_Dir_Entry;
	class IDirectory;
	class CDirectory;
	class CRoot; 
	class CFile; 
	class CFile_System; 
	class CMount;

	struct TSuperblock {
		char name[4]; // 'fat\0'
		kiv_hal::TDrive_Parameters disk_params;
		size_t sectors_per_cluster;
		size_t fat_table_first_cluster;
		size_t fat_table_number_of_entries;
		size_t root_cluster;
		size_t data_first_cluster;
	};

	using TFAT_Entry = uint32_t;

	struct TFAT_Dir_Entry {
		char name[8 + 1 + 3]; // filename.ext
		char fill[3]; // Fill to 24 bytes
		kiv_os::NFile_Attributes attributes;
		TFAT_Entry start; 
		uint32_t filesize;
	};

	class CFAT_Utils {
		public:
			CFAT_Utils(TSuperblock &sb, kiv_vfs::TDisk_Number disk_number);
			CFAT_Utils(kiv_vfs::TDisk_Number disk_number);
			bool Write_To_Disk(char *sectors, uint64_t first_sector, uint64_t num_of_sectors);
			bool Read_From_Disk(char *buffer, uint64_t first_sector, uint64_t num_of_sectors);
			bool Write_Clusters(char *clusters, uint64_t first_cluster, uint64_t num_of_clusters);
			bool Read_Clusters(char *buffer, uint64_t first_cluster, uint64_t num_of_clusters);
			bool Write_Data_Cluster(char *clusters, TFAT_Entry fat_entry);
			bool Read_Data_Cluster(char *buffer, TFAT_Entry fat_entry);
			bool Set_Fat_Entries_Value(std::vector<TFAT_Entry> &entries, TFAT_Entry value);
			bool Get_Free_Fat_Entries(std::vector<TFAT_Entry> &entries, size_t number_of_entries);
			bool Write_Fat_Entries(std::map<TFAT_Entry, TFAT_Entry> &entries);
			bool Get_File_Fat_Entries(TFAT_Entry first_entry, std::vector<TFAT_Entry> &entries);
			bool Free_File_Fat_Entries(TFAT_Dir_Entry &entry);
			bool Load_Directory(std::vector<TFAT_Dir_Entry> dirs_from_root, std::shared_ptr<IDirectory> &directory);
			std::map<TFAT_Entry, TFAT_Entry> Create_Fat_Entries_Chain(std::vector<TFAT_Entry> &entries);

			void Set_Superblock(TSuperblock sb);
			void Set_Root(std::shared_ptr<CRoot> &root);
			TSuperblock &Get_Superblock();
		private:
			TSuperblock mSb;
			kiv_vfs::TDisk_Number mDisk_number;
			std::mutex mDisk_access_lock;
			std::shared_ptr<CRoot> mRoot;
	};

	// Abstract directory (root and subdirectories)
	class IDirectory : public kiv_vfs::IFile {
		public:
			IDirectory(CFAT_Utils *utils);
			virtual size_t Read(char *buffer, size_t buffer_size, size_t position) final override;
			virtual bool Is_Empty() final override;
			virtual std::shared_ptr<kiv_vfs::IFile> Create_File(const kiv_vfs::TPath path, kiv_os::NFile_Attributes attributes);
			virtual bool Remove_File(const kiv_vfs::TPath &path);
			virtual bool Find(std::string filename, TFAT_Dir_Entry &first_entry) final; 
			virtual bool Change_Entry_Size(std::string filename, uint32_t filesize) final;
			virtual bool IDirectory::Get_Entry_Size(std::string filename, uint32_t &filesize) final;

			virtual bool Load() = 0;
			virtual bool Save() = 0;
			virtual std::shared_ptr<kiv_vfs::IFile> Make_File(kiv_vfs::TPath path, TFAT_Dir_Entry entry) = 0;

		protected:
			std::vector<TFAT_Dir_Entry> mEntries;
			uint32_t mSize;
			CFAT_Utils *mUtils;
	};

	// Subdirectory
	class CDirectory : public IDirectory {
		public:
			CDirectory(kiv_vfs::TPath path, TFAT_Dir_Entry &dir_entry, std::vector<TFAT_Dir_Entry> dirs_to_parent, CFAT_Utils *utils);
			CDirectory(TFAT_Dir_Entry &dir_entry, CFAT_Utils *utils);
			virtual bool Load() override final;
			virtual bool Save() override final;
			virtual std::shared_ptr<kiv_vfs::IFile> Make_File(kiv_vfs::TPath path, TFAT_Dir_Entry entry) override final;

		private:
			TFAT_Dir_Entry mDir_entry;
			std::vector<TFAT_Dir_Entry> mDirs_to_parent;
	};

	// Root directory
	class CRoot : public IDirectory {
		public:
			CRoot(CFAT_Utils *utils); 
			virtual bool Load() override final;
			virtual bool Save() override final;
			virtual std::shared_ptr<kiv_vfs::IFile> Make_File(kiv_vfs::TPath path, TFAT_Dir_Entry entry) override final;
	};

	// File
	class CFile : public kiv_vfs::IFile {
		public:
			CFile(const kiv_vfs::TPath path, TFAT_Dir_Entry &dir_entry, std::vector<TFAT_Dir_Entry> dirs_to_parent, CFAT_Utils *utils);
			virtual size_t Write(const char *buffer, size_t buffer_size, size_t position) final override;
			virtual size_t Read(char *buffer, size_t buffer_size, size_t position) final override;
			virtual bool Resize(size_t size) final override;
			virtual bool Is_Available_For_Write() final override;
			virtual size_t Get_Size() final override;

		private:
			std::string filename;
			uint32_t mSize;
			std::vector<TFAT_Entry> mFat_entries;
			std::vector<TFAT_Dir_Entry> mDirs_to_parent;
			CFAT_Utils *mUtils;
	};

	class CFile_System : public kiv_vfs::IFile_System {
		public:
			CFile_System();
			virtual kiv_vfs::IMounted_File_System *Create_Mount(const std::string label, const kiv_vfs::TDisk_Number disk_number) final override;
	};

	class CMount : public kiv_vfs::IMounted_File_System {
		public:
			CMount(std::string label, kiv_vfs::TDisk_Number disk_number);
			~CMount();
			virtual std::shared_ptr<kiv_vfs::IFile> Open_File(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes) final override;
			virtual std::shared_ptr<kiv_vfs::IFile> Create_File(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes) final override;
			virtual bool Delete_File(const kiv_vfs::TPath &path) final override;

		private:
			kiv_vfs::TDisk_Number mDisk_Number;
			TSuperblock mSuperblock{};
			std::shared_ptr<CRoot> root;
			CFAT_Utils *mUtils;

			bool Load_Superblock(kiv_hal::TDrive_Parameters &params);
			bool Chech_Superblock();
			bool Format_Disk(kiv_hal::TDrive_Parameters &params);
			bool Load_Disk_Params(kiv_hal::TDrive_Parameters &params);
			bool Init_Fat_Table();
			bool Init_Root();
	};

}