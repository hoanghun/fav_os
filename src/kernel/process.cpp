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
			kiv_thread::CThread_Manager::Get_Instance().Exit_Thread(regs);
			break;

		case kiv_os::NOS_Process::Shutdown:
			CProcess_Manager::Get_Instance().Shutdown();

			// TODO move to kernel shutdown
			// Free memory before shutdown
			CProcess_Manager::Destroy();
			kiv_thread::CThread_Manager::Destroy();

			break;

		case kiv_os::NOS_Process::Wait_For:
			//TODO
			break;
		case kiv_os::NOS_Process::Register_Signal_Handler:
			kiv_thread::CThread_Manager::Get_Instance().Add_Terminate_Handler(regs);
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

	std::mutex CProcess_Manager::ptable;

	CProcess_Manager * CProcess_Manager::instance = NULL;

	CProcess_Manager::CProcess_Manager() {
		// Musíme spustit systémový proces, který je rodiè všech ostatních procesù
		Create_Sys_Process();
		pid_manager = CPid_Manager();
	}

	void CProcess_Manager::Destroy() {
		delete instance;
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

		// uzamknuti tabulky procesu tzn. i tabulky vlaken
		// mozna by slo udelat efektivneji nez zamykat celou tabulku
		ptable.lock();
		{
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
		}
		ptable.unlock();

		//TODO write informations to PCB

		kiv_thread::CThread_Manager t_manager = kiv_thread::CThread_Manager::Get_Instance();
		t_manager.Create_Thread(pid, context);

		return true;
	}

	//bool CProcess_Manager::Exit_Process(kiv_hal::TRegisters& context) {

	//	//TODO implement
	//	return false;

	//}

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
		
		ptable.lock();
		{

			auto pcb = std::make_shared<TProcess_Control_Block>();
			Get_Pcb(std::this_thread::get_id(), pcb);

			bool terminated = true;
			size_t position = 0;
			for (auto tcb : pcb->thread_table) {

				if (tcb->state == kiv_thread::NThread_State::TERMINATED) {

					tcb->pcb = nullptr;
					tcb->thread.detach(); // musime pouzit join() nebo detach() predtim nez znicime objekt std::thread
					pcb->thread_table.erase(pcb->thread_table.begin() + position);

				}
				else {
					terminated = false;
				}

				position++;
			}

			if (terminated) {
				process_table.erase(process_table.begin() + pcb->pid);
				pid_manager.Release_Pid(pcb->pid);

				//TODO child processes
			}

		}
		ptable.unlock();

	}

	//  Vytvoøíme systémový init proces
	void CProcess_Manager::Create_Sys_Process() {

		//Na zaèátku musí být urèitì volný pid, proto nenní potøeba kontrolovat 
		size_t pid;
		pid_manager.Get_Free_Pid(&pid);
		
		ptable.lock();
		{
			// Nastavíme PCB pro systémový proces
			std::shared_ptr<TProcess_Control_Block> pcb = std::make_shared<TProcess_Control_Block>();
			pcb->name = "system";
			pcb->pid = pid;
			pcb->ppid = 0; //TODO negative value could be better, but size_t ....
			pcb->state = NProcess_State::RUNNING;

			// Nastavíme vlákno pro systémový proces
			std::shared_ptr<kiv_thread::TThread_Control_Block> tcb = std::make_shared<kiv_thread::TThread_Control_Block>();
			tcb->pcb = pcb;
			tcb->state = kiv_thread::NThread_State::RUNNING;
			tcb->terminate_handler = nullptr;
			tcb->tid = std::this_thread::get_id();

			pcb->thread_table.push_back(tcb);
			process_table.push_back(pcb);
		}
		ptable.unlock();

	}

	void CProcess_Manager::Shutdown() {
		
		kiv_hal::TRegisters registers;
		//TODO set registers to contain informations

		for (const auto pcb : process_table) {
			for (const auto tcb : pcb->thread_table) {

				if (tcb->terminate_handler == nullptr) {
					//NO TIME FOR MERCY, KILL IT!
					tcb->pcb = nullptr;
					kiv_thread::Kiv_Os_Default_Terminate_Handler(tcb);
				}
				else {

					tcb->pcb = NULL;
					tcb->terminate_handler(registers);
					// TODO mohlo by se stat ze se nedockam
					tcb->thread.join();
				}

			}
		}

	}

#pragma endregion

}