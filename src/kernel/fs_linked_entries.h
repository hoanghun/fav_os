#pragma once
#include <mutex>
#include <map>

#include "vfs.h"
#include "../api/api.h"

namespace kiv_fs_linked_entries {

	struct TSuperblock;
	struct TLE_Dir_Entry;
	class IDirectory;
	class CDirectory;
	class CRoot; 
	class CFile; 
	class CFile_System; 
	class CMount;

	struct TSuperblock {
		char name[3]; // 'LE\0'
		kiv_hal::TDrive_Parameters disk_params;
		size_t sectors_per_cluster;
		size_t le_table_first_cluster;
		size_t le_table_number_of_entries;
		size_t root_cluster;
		size_t data_first_cluster;
	};

	using TLE_Entry = uint32_t;

	struct TLE_Dir_Entry {
		char name[8 + 1 + 3]; // filename.ext
		char fill[3]; // Fill to 24 bytes
		kiv_os::NFile_Attributes attributes;
		TLE_Entry start; 
		uint32_t filesize;
	};

	// Utils for mount and files
	class CLE_Utils {
		public:
			CLE_Utils(TSuperblock &sb, kiv_vfs::TDisk_Number disk_number, std::recursive_mutex *fs_lock);
			CLE_Utils(kiv_vfs::TDisk_Number disk_number, std::recursive_mutex *fs_lock);
			bool Write_To_Disk(char *sectors, uint64_t first_sector, uint64_t num_of_sectors);
			bool Read_From_Disk(char *buffer, uint64_t first_sector, uint64_t num_of_sectors);
			bool Write_Clusters(char *clusters, uint64_t first_cluster, uint64_t num_of_clusters);
			bool Read_Clusters(char *buffer, uint64_t first_cluster, uint64_t num_of_clusters);
			bool Write_Data_Cluster(char *clusters, TLE_Entry le_entry);
			bool Read_Data_Cluster(char *buffer, TLE_Entry le_entry);
			bool Set_Le_Entries_Value(std::vector<TLE_Entry> &entries, TLE_Entry value);
			bool Get_Free_Le_Entries(std::vector<TLE_Entry> &entries, size_t number_of_entries);
			bool Write_Le_Entries(std::map<TLE_Entry, TLE_Entry> &entries);
			bool Get_File_Le_Entries(TLE_Entry first_entry, std::vector<TLE_Entry> &entries);
			bool Free_File_Le_Entries(TLE_Dir_Entry &entry);
			bool Load_Directory(std::vector<TLE_Dir_Entry> dirs_from_root, std::shared_ptr<IDirectory> &directory);
			std::map<TLE_Entry, TLE_Entry> Create_Le_Entries_Chain(std::vector<TLE_Entry> &entries);

			void Set_Superblock(TSuperblock sb);
			void Set_Root(std::shared_ptr<CRoot> &root);
			TSuperblock &Get_Superblock();
		private:
			TSuperblock mSb;
			kiv_vfs::TDisk_Number mDisk_number;
			std::recursive_mutex *mFs_lock;
			std::shared_ptr<CRoot> mRoot;
	};

	// Abstract directory (root and subdirectories)
	class IDirectory : public kiv_vfs::IFile {
		public:
			IDirectory(CLE_Utils *utils, std::recursive_mutex *fs_lock);
			virtual kiv_os::NOS_Error Read(char *buffer, size_t buffer_size, size_t position, size_t &read) final override;

			virtual bool Is_Empty() final override;
			virtual std::shared_ptr<kiv_vfs::IFile> Create_File(const kiv_vfs::TPath path, kiv_os::NFile_Attributes attributes);
			virtual bool Remove_File(const kiv_vfs::TPath &path);
			virtual bool Find(std::string filename, TLE_Dir_Entry &first_entry) final; 
			virtual bool Change_Entry_Size(std::string filename, uint32_t filesize) final;
			virtual bool IDirectory::Get_Entry_Size(std::string filename, uint32_t &filesize) final;

			virtual bool Load() = 0;
			virtual bool Save() = 0;
			virtual std::shared_ptr<kiv_vfs::IFile> Make_File(kiv_vfs::TPath path, TLE_Dir_Entry entry) = 0;

		protected:
			std::vector<TLE_Dir_Entry> mEntries;
			uint32_t mSize;
			CLE_Utils *mUtils;
			std::recursive_mutex *mFs_lock;
	};

	// Subdirectory
	class CDirectory : public IDirectory {
		public:
			CDirectory(kiv_vfs::TPath path, TLE_Dir_Entry &dir_entry, std::vector<TLE_Dir_Entry> dirs_to_parent, CLE_Utils *utils, std::recursive_mutex *fs_lock);
			CDirectory(TLE_Dir_Entry &dir_entry, CLE_Utils *utils, std::recursive_mutex *fs_lock);
			virtual bool Load() override final;
			virtual bool Save() override final;
			virtual std::shared_ptr<kiv_vfs::IFile> Make_File(kiv_vfs::TPath path, TLE_Dir_Entry entry) override final;

		private:
			TLE_Dir_Entry mDir_entry;
			std::vector<TLE_Dir_Entry> mDirs_to_parent;
	};

	// Root directory
	class CRoot : public IDirectory {
		public:
			CRoot(CLE_Utils *utils, std::recursive_mutex *fs_lock);
			virtual bool Load() override final;
			virtual bool Save() override final;
			virtual std::shared_ptr<kiv_vfs::IFile> Make_File(kiv_vfs::TPath path, TLE_Dir_Entry entry) override final;
	};

	// File
	class CFile : public kiv_vfs::IFile {
		public:
			CFile(const kiv_vfs::TPath path, TLE_Dir_Entry &dir_entry, std::vector<TLE_Dir_Entry> dirs_to_parent, CLE_Utils *utils, std::recursive_mutex *fs_lock);

			virtual kiv_os::NOS_Error Write(const char *buffer, size_t buffer_size, size_t position, size_t &written) final override;
			virtual kiv_os::NOS_Error Read(char *buffer, size_t buffer_size, size_t position, size_t &read) final override;
			virtual kiv_os::NOS_Error Resize(size_t size) final override;
			virtual bool Is_Available_For_Write() final override;
			virtual size_t Get_Size() final override;

		private:
			std::string filename;
			uint32_t mSize;
			std::vector<TLE_Entry> mLe_entries;
			std::vector<TLE_Dir_Entry> mDirs_to_parent;
			CLE_Utils *mUtils;
			std::recursive_mutex *mFs_lock;
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
			virtual kiv_os::NOS_Error Open_File(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes, std::shared_ptr<kiv_vfs::IFile> &file) final override;
			virtual kiv_os::NOS_Error Create_File(const kiv_vfs::TPath &path, kiv_os::NFile_Attributes attributes, std::shared_ptr<kiv_vfs::IFile> &file) final override;
			virtual kiv_os::NOS_Error Delete_File(const kiv_vfs::TPath &path) final override;

		private:
			kiv_vfs::TDisk_Number mDisk_Number;
			TSuperblock mSuperblock{};
			std::shared_ptr<CRoot> root;
			CLE_Utils *mUtils;
			std::recursive_mutex *mFs_lock;

			bool Load_Superblock(kiv_hal::TDrive_Parameters &params);
			bool Chech_Superblock();
			bool Format_Disk(kiv_hal::TDrive_Parameters &params);
			bool Load_Disk_Params(kiv_hal::TDrive_Parameters &params);
			bool Init_Le_Table();
			bool Init_Root();
	};

}