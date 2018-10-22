#include "source_manager.h"

kiv_process::CSource_Manager *kiv_process::CSource_Manager::instance_ = NULL;

void kiv_process::CSource_Manager::Add_Handle(kiv_os::THandle handle, kiv_process::NHandle_Type type,std::shared_ptr<TControl_Block> block) {
	THandle_Object block_info;
	block_info.control_block = block;
	block_info.type = type;
	block_info.owner = block->owner;

	handles_[handle] = block_info;
}

kiv_process::THandle_Object& kiv_process::CSource_Manager::Get_Handle(kiv_os::THandle handle) {
	return handles_[handle];
}

size_t kiv_process::CSource_Manager::Remove_Handle(kiv_os::THandle handle){
	return handles_.erase(handle);
}

kiv_process::CSource_Manager& kiv_process::CSource_Manager::Get_Instance() {
	if (instance_ == NULL) {
		instance_ = new kiv_process::CSource_Manager();
	}

	return *instance_;
}