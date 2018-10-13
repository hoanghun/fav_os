#include <Windows.h>
#include <memory>

#include "thread.h"
#include "process.h"
#include "common.h"
#include "..\api\api.h"

namespace kiv_thread {

		CThread_Manager * CThread_Manager::instance = NULL;

		CThread_Manager::CThread_Manager() {
		}

		CThread_Manager::~CThread_Manager() {
		}

		CThread_Manager & CThread_Manager::Get_Instance() {

			if (instance == NULL) {
				instance = new CThread_Manager();
			}

			return *instance;

		}

		bool CThread_Manager::Create_Thread(const size_t pid, const char* func_name, const kiv_hal::TRegisters& context) {

			std::shared_ptr<kiv_process::TProcess_Control_Block> pcb = kiv_process::CProcess_Manager::Get_Instance().process_table[pid];

			kiv_os::TThread_Proc func = (kiv_os::TThread_Proc) GetProcAddress(User_Programs, func_name);
	
			std::thread new_thread(func, context);
			new_thread.detach();
			
			std::shared_ptr<TThread_Control_Block> tcb = std::make_shared<TThread_Control_Block>();

			tcb->pcb = pcb;
			tcb->state = NThread_State::RUNNING;
			tcb->tid = new_thread.get_id();

			pcb->thread_table.push_back(tcb);

			return true;
		}

		bool  CThread_Manager::Create_Thread(const kiv_hal::TRegisters& context) {
			
			const char* func_name = (char*)(context.rdx.r);

			size_t pid;
			std::shared_ptr<kiv_process::TProcess_Control_Block> pcb;

			if (!kiv_process::CProcess_Manager::Get_Instance().Get_Pid(std::this_thread::get_id(), &pid, pcb)) {
				//TODO error
			}

			Create_Thread(pid, func_name, context);

			return false;
		}

		bool CThread_Manager::Exit_Thread() {
			//TODO implement
			return false;
		}
}