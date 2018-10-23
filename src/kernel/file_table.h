#pragma once

#include <list>
#include <string>

namespace kiv_io {
	enum class NAccess_Resctriction {
		Read_Only = 1,
		Read_Write,
		Write_Only,
	};

	struct TGFT_Record {
		std::string file_name;
		NAccess_Resctriction access;
		unsigned int offset;
		unsigned int ref_count;
	};

	class CFile_Table {
	public:
		void Remove_Reference(TGFT_Record &record);
		TGFT_Record* Open_File(std::string file_name, NAccess_Resctriction access);

		static CFile_Table& Get_Instance();
		static void Destroy();

	private:
		std::list<TGFT_Record> mGlobal_File_Table;
		static CFile_Table *mInstance;
		bool Remove_Record(TGFT_Record &record);
		CFile_Table() = default;
	};
}