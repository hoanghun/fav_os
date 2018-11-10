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
			kiv_thread::CThread_Manager::Get_Instance().Wait_For(regs);
			break;
		case kiv_os::NOS_Process::Register_Signal_Handler:
			kiv_thread::CThread_Manager::Get_Instance().Add_Terminate_Handler(regs);
			break;
		case kiv_os::NOS_Process::Read_Exit_Code:
			kiv_thread::CThread_Manager::Get_Instance().Read_Exit_Code(regs);
			break;
		}
	}

	void Handle_Clone_Call(kiv_hal::TRegisters &regs) {
		switch (static_cast<kiv_os::NClone>(regs.rcx.r)) {
		case kiv_os::NClone::Create_Process:
			kiv_process::CProcess_Manager::Get_Instance().Create_Process(regs);
			break;

		case kiv_os::NClone::Create_Thread:
			kiv_thread::CThread_Manager::Get_Instance().Create_Thread(regs);
			break;
		}
	}

#pragma region CPid_Manager

	CPid_Manager::CPid_Manager() {
		pids[0] = false;
	}

	bool CPid_Manager::Get_Free_Pid(size_t* pid) {
		std::unique_lock<std::mutex> lock(plock);
		{
			if (!is_full) {

				last = (last + 1) % pids.size();
				size_t start = last;

				do {

					if (pids[last] == false) {
						pids[last] = true;

						*pid = last;

						lock.unlock();
						return true;
					}

					last = (last + 1) % pids.size();

				} while (last != start);

				is_full = true;

			}
		}
		plock.unlock();
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
	
	//Pokud je rovnou nastaveno na instanci, tak se zasekne nacitani DLLka
	CProcess_Manager *CProcess_Manager::instance = nullptr;

	CProcess_Manager::CProcess_Manager() {
		// Musíme spustit systémový proces, který je rodiè všech ostatních procesù
		Create_Sys_Process();
		pid_manager = new CPid_Manager();
	}

	void CProcess_Manager::Destroy() {
		delete instance->pid_manager;
		delete instance;
	}

	CProcess_Manager& CProcess_Manager::Get_Instance() {

		if (instance == nullptr) {
			instance = new CProcess_Manager();
		}

		return *instance;
	}

	bool CProcess_Manager::Create_Process(kiv_hal::TRegisters& context) {

		size_t pid;
		if (!pid_manager->Get_Free_Pid(&pid)) {
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
			pcb->name = std::string((char*)(context.rdx.r));

			std::shared_ptr<TProcess_Control_Block> ppcb = std::make_shared<TProcess_Control_Block>();
			if (!Get_Pcb(kiv_thread::Hash_Thread_Id(std::this_thread::get_id()), ppcb)) {
				return false;
			}

			pcb->ppid = ppcb->pid;
			pcb->working_directory = ppcb->working_directory;
			ppcb->cpids.push_back(pid);

			//CHECK 

			//try {
			//	kiv_os::THandle fd_index = 0;
			//	kiv_vfs::CVirtual_File_System::Get_Instance().Open_File("stdio:stdin", kiv_os::NFile_Attributes::System_File, fd_index);
			//	context.rbx.e = fd_index;
			//	kiv_vfs::CVirtual_File_System::Get_Instance().Open_File("stdio:stdout", kiv_os::NFile_Attributes::System_File, fd_index);
			//	context.rbx.e = (context.rbx.e << 16) | fd_index;
			//}
			//catch (kiv_vfs::TFile_Not_Found_Exception) {
			//	//TODO
			//}
			//catch (kiv_vfs::TPermission_Denied_Exception) {
			//	//TODO
			//}
			
			process_table.push_back(pcb);
		}
		lock.unlock();

		//kiv_thread::CThread_Manager t_manager = kiv_thread::CThread_Manager::Get_Instance();
		//t_manager.Create_Thread(pid, context);
		kiv_thread::CThread_Manager::Get_Instance().Create_Thread(pid, context);

		return true;
	}

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

	//Zkontroluje zda process skoncil tzn. vsechny vlakna jsou ve stavu terminated
	//a provede potrebne akce
	void CProcess_Manager::Check_Process_State(std::shared_ptr<TProcess_Control_Block> pcb) {
		
		std::unique_lock<std::mutex> lock(ptable);
		{

			bool terminated = true;
			size_t position = 0;
			for (auto tcb : pcb->thread_table) {

				//Pokud vlakno skoncilo tak ho smazeme
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

			//Pokud jiz nebezi zadne vlakno v procesu tzn. proces je ukoncen
			if (terminated) {

				//Uzavirani vsech otevrenych souboru
				for (auto &item : pcb->fd_table) {
					kiv_vfs::CVirtual_File_System::Get_Instance().Close_File(item.second);
				}

				pcb->fd_table.clear();

				//Pokud bezi nejake child procesy tak je predame rodici
				for (size_t cpid : pcb->cpids) {
					if (process_table[cpid]->state != NProcess_State::TERMINATED) {
						process_table[pcb->ppid]->cpids.push_back(cpid);
					}
				}

				process_table.erase(process_table.begin() + pcb->pid);
				pid_manager->Release_Pid(pcb->pid);

			}

		}
		lock.unlock();

	}

	void CProcess_Manager::Shutdown() {
		
		kiv_hal::TRegisters regs;
		//TODO set registers to contain informations

		//Zastavi vsechny systemove procesy
		system_shutdown = true;

		for (const auto pcb : process_table) {
			for (const auto tcb : pcb->thread_table) {

				//Systemove procesy nebudeme ukoncovat nasilne
				uint16_t exit_code;
				if (pcb->pid == 0) {
					tcb->thread.join();
					kiv_thread::CThread_Manager::Get_Instance().Read_Exit_Code(tcb->tid, exit_code);
					tcb->pcb = nullptr;
				}
				if (tcb->terminate_handler == nullptr) {
					//NO TIME FOR MERCY, KILL IT!
					kiv_thread::Kiv_Os_Default_Terminate_Handler(tcb);
					kiv_thread::CThread_Manager::Get_Instance().Read_Exit_Code(tcb->tid, exit_code);
					tcb->pcb = nullptr;
				}
				else {
					tcb->terminate_handler(regs);
					// TODO mohlo by se stat ze se nedockam
					tcb->thread.join();
					kiv_thread::CThread_Manager::Get_Instance().Read_Exit_Code(tcb->tid, exit_code);
					tcb->pcb = nullptr;
				}

			}
		}

	}

	bool CProcess_Manager::Set_Working_Directory(const size_t &tid, const kiv_vfs::TPath &dir) {

		std::shared_ptr<kiv_thread::TThread_Control_Block> tcb;

		if (kiv_thread::CThread_Manager::Get_Instance().Get_Thread_Control_Block(tid, &tcb)) {
			std::unique_lock<std::mutex> lock(ptable);
			{
				tcb->pcb->working_directory = dir;
			}
			lock.unlock();
		}
		else {
			return false;
		}

		return true;
	}

	bool CProcess_Manager::Get_Working_Directory(const size_t &tid, kiv_vfs::TPath * dir) const {

		std::shared_ptr<kiv_thread::TThread_Control_Block> tcb;

		if (kiv_thread::CThread_Manager::Get_Instance().Get_Thread_Control_Block(tid, &tcb)) {
			*dir = tcb->pcb->working_directory;
		}
		else {
			return false;
		}

		return true;

	}

	unsigned int CProcess_Manager::Save_Fd(const std::shared_ptr<TProcess_Control_Block> &pcb, const kiv_os::THandle &fd_index) {

		std::unique_lock<std::mutex> lock(ptable);
		{
			pcb->fd_table.emplace(pcb->last_fd++, fd_index);
		}
		lock.unlock();

		return pcb->last_fd - 1;

	}

	bool CProcess_Manager::Save_Fd(const kiv_os::THandle &fd_index) {

		std::shared_ptr<kiv_thread::TThread_Control_Block> tcb;

		if (kiv_thread::CThread_Manager::Get_Instance().Get_Thread_Control_Block(kiv_thread::Hash_Thread_Id(std::this_thread::get_id()) , &tcb)) {
			return Save_Fd(tcb->pcb, fd_index);
		}
		else {
			return false;
		}
	}

	void CProcess_Manager::Remove_Fd(const std::shared_ptr<TProcess_Control_Block> &pcb, const kiv_os::THandle &fd_index) {
		pcb->fd_table.erase(fd_index);
	}

	bool CProcess_Manager::Remove_Fd(const kiv_os::THandle &fd_index) {

		std::shared_ptr<kiv_thread::TThread_Control_Block> tcb;

		if (kiv_thread::CThread_Manager::Get_Instance().Get_Thread_Control_Block(kiv_thread::Hash_Thread_Id(std::this_thread::get_id()) , &tcb)) {
			Remove_Fd(tcb->pcb, fd_index);
			return true;
		}
		else {
			return false;
		}

	}

	bool CProcess_Manager::Get_Fd(const size_t &position, kiv_os::THandle &fd) {

		std::shared_ptr<kiv_thread::TThread_Control_Block> tcb;

		if (kiv_thread::CThread_Manager::Get_Instance().Get_Thread_Control_Block(kiv_thread::Hash_Thread_Id(std::this_thread::get_id()), &tcb)) {
			
			if (tcb->pcb->fd_table.size() < position) {
				return false;
			}
			else {
				fd = tcb->pcb->fd_table.at(position);
				return true;
			}	
		}
		else {
			return false;
		}

	}

#pragma region System_Processes

	//  Vytvoøíme systémový init proces
	void CProcess_Manager::Create_Sys_Process() {

		//Na zaèátku musí být urèitì volný pid, proto nenní potøeba kontrolovat 
		
		std::unique_lock<std::mutex> lock(ptable);
		{
			// Nastavíme PCB pro systémový proces
			std::shared_ptr<TProcess_Control_Block> pcb = std::make_shared<TProcess_Control_Block>();
			pcb->name = "system";
			pcb->pid = 0;
			pcb->ppid = 0; //CHECK negative value could be better, but size_t ....
			pcb->state = NProcess_State::RUNNING;
			//TODO pcb->working_directory = DEFAULT;

			// Nastavíme vlákno pro systémový proces
			std::shared_ptr<kiv_thread::TThread_Control_Block> tcb = std::make_shared<kiv_thread::TThread_Control_Block>();
			tcb->pcb = pcb;
			tcb->state = kiv_thread::NThread_State::RUNNING;
			tcb->terminate_handler = nullptr;
			//CHECK pri nacitani dll se zasekne pokud ihned inicializujeme instanci
			tcb->thread = std::thread(&CProcess_Manager::Reap_Process, this);
			tcb->tid = kiv_thread::Hash_Thread_Id(std::this_thread::get_id());

			std::unique_lock<std::mutex> tm_lock(kiv_thread::CThread_Manager::Get_Instance().maps_lock);
			{
				std::shared_ptr<kiv_thread::TThread_Control_Block> ptr = tcb;
				kiv_thread::CThread_Manager::Get_Instance().thread_map.emplace(tcb->tid, tcb);
			}
			tm_lock.unlock();

			pcb->thread_table.push_back(tcb);
			process_table.push_back(pcb);
		}
		lock.unlock();

	}

	//Stara se o procesy, kterym skoncil rodicovsky proces aniz by si precetl
	//navratovy kod
	void CProcess_Manager::Reap_Process() {

		std::vector<size_t> child_pids;
		std::vector<size_t> handles;

		while (!system_shutdown) {

			if (ptable.try_lock()) {
				
				child_pids = process_table[0]->cpids;
				handles.clear();

				for (const size_t cpid : child_pids) {
					std::shared_ptr<TProcess_Control_Block> cpcb = process_table[cpid];

					for (const auto &tcb : cpcb->thread_table) {
						handles.push_back(tcb->tid);
					}
				}
				ptable.unlock();
			}
			else
			{
				//Pokud nedostaneme zamek nad process_table nema cenu pokracovat, ale chceme se dostat k praci co nejdrive
				std::this_thread::yield();
			}
			

			uint16_t exit_code;
			for (const size_t handle : handles) {
				kiv_thread::CThread_Manager::Get_Instance().Read_Exit_Code(handle, exit_code);
			}

			std::this_thread::yield();
		}

	}

#pragma endregion

#pragma endregion

}