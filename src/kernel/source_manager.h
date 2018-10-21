#pragma once

#include "process.h"

#include <map>
#include <memory>

namespace kiv_process {
	struct Control_Block;

	enum class Handle_Type {
		FILE = 1,
		PROCESS,
	};

	struct Handle {
		Handle_Type type;
		std::shared_ptr<Control_Block> control_block;
		size_t owner;
	};

	class Source_Manager {
	public:
		void Add_Handle(kiv_os::THandle handle, Handle_Type type, std::shared_ptr<kiv_process::Control_Block> block);
		Handle& Get_Handle(kiv_os::THandle handle);
		size_t Remove_Handle(kiv_os::THandle handle);
		static Source_Manager& Get_Instance();

	private:
		std::map<kiv_os::THandle, Handle> handles_;
		static Source_Manager *instance_;
		
		Source_Manager() {}
	};
}