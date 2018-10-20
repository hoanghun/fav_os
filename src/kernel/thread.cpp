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

		bool CThread_Manager::Create_Thread(const size_t pid, kiv_hal::TRegisters& context) {

			const char* func_name = (char*)(context.rdx.r);

			std::shared_ptr<kiv_process::TProcess_Control_Block> pcb = kiv_process::CProcess_Manager::Get_Instance().process_table[pid];

			kiv_os::TThread_Proc func = (kiv_os::TThread_Proc) GetProcAddress(User_Programs, func_name);

			if (!func) {
				return false;
			}
	
			
			//TODO lock table
			std::shared_ptr<TThread_Control_Block> tcb = std::make_shared<TThread_Control_Block>();
	
			tcb->thread = std::thread(func, context);

			tcb->pcb = pcb;
			tcb->state = NThread_State::RUNNING;
			tcb->tid = tcb->thread.get_id();
			tcb->terminate_handler = nullptr;


			pcb->thread_table.push_back(tcb);
			//TODO unlock table

			//TODO REMOVE
			tcb->thread.join();

			return true;
		}

		bool  CThread_Manager::Create_Thread(kiv_hal::TRegisters& context) {

			std::shared_ptr<kiv_process::TProcess_Control_Block> pcb = std::make_shared<kiv_process::TProcess_Control_Block>();

			if (!kiv_process::CProcess_Manager::Get_Instance().Get_Pcb(std::this_thread::get_id(), pcb)) {
				//TODO error
				return false;
			}

			Create_Thread(pcb->pid, context);

			return false;
		}

		bool CThread_Manager::Exit_Thread(kiv_hal::TRegisters& context) {
			
			std::shared_ptr<TThread_Control_Block> tcb = std::make_shared<TThread_Control_Block>();

			if (!kiv_process::CProcess_Manager::Get_Instance().Get_Tcb(std::this_thread::get_id(), tcb)) {
				return false;
			}

			if (tcb->terminate_handler == nullptr) {
				return false;
			}

			kiv_hal::TRegisters& regs = kiv_hal::TRegisters();
			//TODO fill registers

			tcb->terminate_handler(regs);
			tcb->state = NThread_State::TERMINATED;

			kiv_process::CProcess_Manager::Get_Instance().Check_Process_State(tcb->pcb->pid);

			return true;
		}

		bool CThread_Manager::Add_Terminate_Handler(const kiv_hal::TRegisters& context) {

			std::shared_ptr<TThread_Control_Block> tcb = std::make_shared<TThread_Control_Block>();

			if (!kiv_process::CProcess_Manager::Get_Instance().Get_Tcb(std::this_thread::get_id(), tcb)) {
				return false;
			}

			tcb->terminate_handler = 0; //TODO add address from context

			return true;

		}

}