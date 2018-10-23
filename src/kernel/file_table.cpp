#include "file_table.h"

kiv_io::CFile_Table *kiv_io::CFile_Table::mInstance = new kiv_io::CFile_Table();


namespace kiv_io {
	CFile_Table& CFile_Table::Get_Instance() {
		return *mInstance;
	}

	void CFile_Table::Remove_Reference(TGFT_Record &record) {
		record.ref_count -= 1;
		if (record.ref_count <= 0) {
			Remove_Record(record);
		}
	}

	bool CFile_Table::Remove_Record(TGFT_Record &record) {
		for (auto it = mGlobal_File_Table.begin(); it != mGlobal_File_Table.end(); ++it) {
			if ((*it).file_name == record.file_name) {
				mGlobal_File_Table.erase(it);
				
				return true;
			}
		}

		return false;
	}

	TGFT_Record* CFile_Table::Open_File(std::string file_name, NAccess_Resctriction access) {
		for (auto it = mGlobal_File_Table.begin(); it != mGlobal_File_Table.end(); ++it) {
			if ((*it).file_name == file_name) {
				if (it->access != access) {
					return NULL;
				}

				it->ref_count += 1;
				return &(*it);
			}
		}

		TGFT_Record record;
		record.file_name = file_name;
		record.access = access;
		record.ref_count = 1;
		record.offset = 0;

		mGlobal_File_Table.push_back(record);
		return &mGlobal_File_Table.back();
	}

	void CFile_Table::Destroy() {
		delete mInstance;
	}
}