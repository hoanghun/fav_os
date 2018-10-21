#include "source_manager.h"

kiv_process::Source_Manager *kiv_process::Source_Manager::instance_ = NULL;

void kiv_process::Source_Manager::Add_Handle(kiv_os::THandle handle, kiv_process::Handle_Type type,std::shared_ptr<Control_Block> block) {
	Handle block_info;
	block_info.control_block = block;
	block_info.type = type;
	block_info.owner = block->owner;

	handles_[handle] = block_info;
}

kiv_process::Handle& kiv_process::Source_Manager::Get_Handle(kiv_os::THandle handle) {
	return handles_[handle];
}

size_t kiv_process::Source_Manager::Remove_Handle(kiv_os::THandle handle){
	return handles_.erase(handle);
}

kiv_process::Source_Manager& kiv_process::Source_Manager::Get_Instance() {
	if (instance_ == NULL) {
		instance_ = new kiv_process::Source_Manager();
	}

	return *instance_;
}