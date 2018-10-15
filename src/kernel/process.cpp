#include <cstring>

#include "process.h"
#include "common.h"

namespace kiv_process {

	void Handle_Process(kiv_hal::TRegisters &regs) {
		switch (static_cast<kiv_os::NOS_Process>(regs.rax.l)) {
		case kiv_os::NOS_Process::Clone:
			CProcess_Manager::Get_Instance().Create_Process(regs);
			break;
		case kiv_os::NOS_Process::Exit:
			//TODO
			break;

		case kiv_os::NOS_Process::Shutdown:
			//TODO
			break;

		case kiv_os::NOS_Process::Wait_For:
			//TODO
			break;
		}
	}

	void Handle_Clone_Call(kiv_hal::TRegisters &regs) {
		switch (static_cast<kiv_os::NClone>(regs.rcx.r)) {
		case kiv_os::NClone::Create_Process:
			kiv_process::CProcess_Manager::Get_Instance().Create_Process(regs);
			break;

		case kiv_os::NClone::Create_Thread:
			// TODO call to create new thread
			break;
		}
	}

#pragma region CPid_Manager
	bool CPid_Manager::Get_Free_Pid(size_t* pid) {
		if (!is_full) {

			last = (last + 1) % pids.size();
			size_t start = last;

			do {

				if (pids[last] == false) {
					pids[last] = true;

					*pid = last;
					return true;
				}

				last = (last + 1) % pids.size();

			} while (last != start);

			is_full = true;
			
		}

		return false;
	}

	bool CPid_Manager::Release_Pid(size_t pid) {

		if (pid > 0 && pid < pids.size() - 1) {
			
			pids[pid] = false;
			is_full = false;

			return true;
		}
		else {
			return false;
		}

	}

#pragma endregion

#pragma region CProcess_Manager

	CProcess_Manager * CProcess_Manager::instance = NULL;

	CProcess_Manager::CProcess_Manager() {
		// Mus�me spustit syst�mov� proces, kter� je rodi� v�ech ostatn�ch proces�
		Create_Sys_Process();
		pid_manager = CPid_Manager();
	}

	CProcess_Manager::~CProcess_Manager() {

	}

	CProcess_Manager & CProcess_Manager::Get_Instance() {

		if (instance == NULL) {
			instance = new CProcess_Manager();
		}

		return *instance;

	}

	bool CProcess_Manager::Create_Process(kiv_hal::TRegisters& context) {
		const char* prog_name = (char*)(context.rdx.r);

		size_t pid;
		if (!pid_manager.Get_Free_Pid(&pid)) {
			// TODO process cannot be created
			return false;
		}

		std::shared_ptr<TProcess_Control_Block> pcb = std::make_shared<TProcess_Control_Block>();

		pcb->pid = pid;
		pcb->state = NProcess_State::RUNNING;
		// TODO name

		std::shared_ptr<TProcess_Control_Block> ppcb = std::make_shared<TProcess_Control_Block>();
		if (!Get_Pcb(std::this_thread::get_id(), ppcb)) {
			return false;
		}

		
		pcb->ppid = ppcb->pid;
		ppcb->cpids.push_back(pid);

		process_table.push_back(pcb);
		//TODO write informations to PCB

		kiv_thread::CThread_Manager t_manager = kiv_thread::CThread_Manager::Get_Instance();
		t_manager.Create_Thread(pid, prog_name, context);

		return true;
	}

	bool CProcess_Manager::Exit_Process(kiv_hal::TRegisters& context) {

		//TODO implement
		return fabsl;

	}

	bool CProcess_Manager::Get_Pcb(std::thread::id tid, std::shared_ptr<TProcess_Control_Block> pcb) {

		for(std::shared_ptr<TProcess_Control_Block> tpcb : process_table) {

			for (std::shared_ptr<kiv_thread::TThread_Control_Block> ttcb : tpcb->thread_table) {
				if (ttcb->tid == tid) {
					
					if (pcb != nullptr) {
						pcb = tpcb;
					}
					
					return true;
				}
			}

		}

		return false;
	}

	bool CProcess_Manager::Get_Tcb(std::thread::id tid, std::shared_ptr<kiv_thread::TThread_Control_Block> tcb) {

		for (std::shared_ptr<TProcess_Control_Block> tpcb : process_table) {

			for (std::shared_ptr<kiv_thread::TThread_Control_Block> ttcb : tpcb->thread_table) {
				if (ttcb->tid == tid) {

					if (tcb != nullptr) {
						tcb = ttcb;
					}

					return true;
				}
			}

		}

		return false;

	}

	void CProcess_Manager::Check_Process_State(size_t pid) {
		//TODO
	}

	//  Vytvo��me syst�mov� init proces
	void CProcess_Manager::Create_Sys_Process() {

		//Na za��tku mus� b�t ur�it� voln� pid, proto nenn� pot�eba kontrolovat 
		size_t pid;
		pid_manager.Get_Free_Pid(&pid);
		
		// Nastav�me PCB pro syst�mov� proces
		std::shared_ptr<TProcess_Control_Block> pcb = std::make_shared<TProcess_Control_Block>();
		pcb->name = "system";
		pcb->pid = pid;
		pcb->ppid = 0; //TODO negative value could be better, but size_t ....
		pcb->state = NProcess_State::RUNNING;

		// Nastav�me vl�kno pro syst�mov� proces
		std::shared_ptr<kiv_thread::TThread_Control_Block> tcb = std::make_shared<kiv_thread::TThread_Control_Block>();
		tcb->pcb = pcb;
		tcb->state = kiv_thread::NThread_State::RUNNING;
		tcb->terminate_handler = nullptr;
		tcb->tid = std::this_thread::get_id();

		pcb->thread_table.push_back(tcb);
		process_table.push_back(pcb);
	}

#pragma endregion

}