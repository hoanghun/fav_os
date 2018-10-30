#include <cstring>

#include "process.h"
#include "common.h"

#include <iostream>
namespace kiv_process {

	bool system_shutdown = false;
	int waiting_time = 50;

	void Handle_Process(kiv_hal::TRegisters &regs) {
		switch (static_cast<kiv_os::NOS_Process>(regs.rax.l)) {

		case kiv_os::NOS_Process::Clone:
			CProcess_Manager::Get_Instance().Create_Process(regs);
			break;
		case kiv_os::NOS_Process::Exit:
			kiv_thread::CThread_Manager::Get_Instance().Thread_Exit(regs);
			break;

		case kiv_os::NOS_Process::Shutdown:
			CProcess_Manager::Get_Instance().Shutdown();

			// TODO move to kernel shutdown
			// Free memory before shutdown
		
			CProcess_Manager::Destroy();
			kiv_thread::CThread_Manager::Destroy();
			kiv_vfs::CVirtual_File_System::Destroy();

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

	CProcess_Manager *CProcess_Manager::instance = new CProcess_Manager();

	CProcess_Manager::CProcess_Manager() {
		// Mus�me spustit syst�mov� proces, kter� je rodi� v�ech ostatn�ch proces�
		Create_Sys_Process();
		pid_manager = CPid_Manager();
	}

	void CProcess_Manager::Destroy() {
		delete instance;
	}

	CProcess_Manager& CProcess_Manager::Get_Instance() {
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
		std::unique_lock<std::mutex> lock(ptable);
		{
			std::shared_ptr<TProcess_Control_Block> pcb = std::make_shared<TProcess_Control_Block>();

			pcb->pid = pid;
			pcb->state = NProcess_State::RUNNING;
			// TODO name

			std::shared_ptr<TProcess_Control_Block> ppcb = std::make_shared<TProcess_Control_Block>();
			if (!Get_Pcb(kiv_thread::Hash_Thread_Id(std::this_thread::get_id()), ppcb)) {
				return false;
			}

			pcb->ppid = ppcb->pid;
			ppcb->cpids.push_back(pid);

			process_table.push_back(pcb);
		}
		lock.unlock();

		//TODO write informations to PCB

		//kiv_thread::CThread_Manager t_manager = kiv_thread::CThread_Manager::Get_Instance();
		//t_manager.Create_Thread(pid, context);
		kiv_thread::CThread_Manager::Get_Instance().Create_Thread(pid, context);

		return true;
	}

	//bool CProcess_Manager::Exit_Process(kiv_hal::TRegisters& context) {

	//	//TODO implement
	//	return false;

	//}

	bool CProcess_Manager::Get_Pcb(size_t tid, std::shared_ptr<TProcess_Control_Block> pcb) {
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

	bool CProcess_Manager::Get_Tcb(size_t tid, std::shared_ptr<kiv_thread::TThread_Control_Block> tcb) {
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

	//TODO doladit co vsechno ma delat
	void CProcess_Manager::Check_Process_State(std::shared_ptr<TProcess_Control_Block> pcb) {
		
		ptable.lock();
		{

			bool terminated = true;
			size_t position = 0;
			for (auto tcb : pcb->thread_table) {

				if (tcb->state == kiv_thread::NThread_State::TERMINATED) {

					tcb->pcb = nullptr;
					tcb->thread.join(); // musime pouzit join() nebo detach() predtim nez znicime objekt std::thread
					pcb->thread_table.erase(pcb->thread_table.begin() + position);

				}
				else {
					terminated = false;
				}

				position++;
			}

			if (terminated) {

				//If there are not terminated child processes
				for (size_t cpid : pcb->cpids) {
					if (process_table[cpid]->state != NProcess_State::TERMINATED) {
						process_table[pcb->ppid]->cpids.push_back(cpid);
					}
				}

				process_table.erase(process_table.begin() + pcb->pid);
				pid_manager.Release_Pid(pcb->pid);

			}

		}
		ptable.unlock();

	}

	void CProcess_Manager::Shutdown() {
		
		kiv_hal::TRegisters registers;
		//TODO set registers to contain informations

		//Zastavi vsechny systemove procesy
		system_shutdown = true;

		for (const auto pcb : process_table) {
			for (const auto tcb : pcb->thread_table) {

				//Systemove procesy nebudeme ukoncovat nasilne
				if (pcb->pid == 0) {
					tcb->thread.join();
					tcb->pcb = NULL;
				}
				if (tcb->terminate_handler == nullptr) {
					//NO TIME FOR MERCY, KILL IT!
					tcb->pcb = nullptr;
					kiv_thread::Kiv_Os_Default_Terminate_Handler(tcb);
				}
				else {
					tcb->terminate_handler(registers);
					// TODO mohlo by se stat ze se nedockam
					tcb->thread.join();
					tcb->pcb = NULL;
				}

			}
		}

	}

#pragma region System_Processes

	//  Vytvo��me syst�mov� init proces
	void CProcess_Manager::Create_Sys_Process() {

		//Na za��tku mus� b�t ur�it� voln� pid, proto nenn� pot�eba kontrolovat 
		size_t pid;
		pid_manager.Get_Free_Pid(&pid);
		
		ptable.lock();
		{
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
			tcb->thread = std::thread(&CProcess_Manager::Reap_Process, this);
			tcb->tid = kiv_thread::Hash_Thread_Id(std::this_thread::get_id());

			pcb->thread_table.push_back(tcb);
			process_table.push_back(pcb);
		}
		ptable.unlock();

	}

	//Stara se o procesy
	void CProcess_Manager::Reap_Process() {

		while (!system_shutdown) {

			if (ptable.try_lock()) {
				//TODO do your job
			}
			else
			{
				//Pokud nedostaneme zamek nad process_table nema cenu pokracovat, ale chceme se dostat k praci co nejdrive
				std::this_thread::yield();
			}
			ptable.unlock();

			std::this_thread::sleep_for(std::chrono::milliseconds(waiting_time));
		}

	}

#pragma endregion

#pragma endregion

}