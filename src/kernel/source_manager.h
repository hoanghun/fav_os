#pragma once

#include "process.h"

#include <map>
#include <memory>

namespace kiv_process {
	struct TControl_Block;

	enum class NHandle_Type {
		FILE = 1,
		PROCESS,
	};

	struct THandle_Object {
		NHandle_Type type;
		std::shared_ptr<TControl_Block> control_block;
		size_t owner;
	};

	class CSource_Manager {
	public:
		void Add_Handle(kiv_os::THandle handle, NHandle_Type type, std::shared_ptr<kiv_process::TControl_Block> block);
		THandle_Object& Get_Handle(kiv_os::THandle handle);
		size_t Remove_Handle(kiv_os::THandle handle);
		static CSource_Manager& Get_Instance();

	private:
		std::map<kiv_os::THandle, THandle_Object> handles_;
		static CSource_Manager *instance_;
		
		CSource_Manager() {}
	};
}