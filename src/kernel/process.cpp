#include <cstring>

#include "process.h"
#include "common.h"

#include <iostream>
namespace kiv_process {

#pragma region "SYSTEM VARIABLES"

	bool system_shutdown = false;
	
	std::mutex reap_mutex;
	std::condition_variable reap_cv;

#pragma endregion

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

	void Handle_Process(kiv_hal::TRegisters &regs) {
		switch (static_cast<kiv_os::NOS_Process>(regs.rax.l)) {

		case kiv_os::NOS_Process::Clone:
			Handle_Clone_Call(regs);
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

		const char* func_name = (char*)(context.rdx.r);
		kiv_os::TThread_Proc func = (kiv_os::TThread_Proc) GetProcAddress(User_Programs, func_name);

		if (!func) {
			context.rax.r = static_cast<uint64_t>(kiv_os::NOS_Error::Invalid_Argument);
			context.flags.carry = 1;
			return false;
		}

		size_t pid;
		if (!pid_manager->Get_Free_Pid(&pid)) {
			context.rax.r = static_cast<uint64_t>(kiv_os::NOS_Error::Out_Of_Memory);
			context.flags.carry = 1;
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

			std::shared_ptr<TProcess_Control_Block> ppcb;
			if (!Get_Pcb(kiv_thread::Hash_Thread_Id(std::this_thread::get_id()), &ppcb)) {
				context.rax.r = static_cast<uint64_t>(kiv_os::NOS_Error::Unknown_Error);
				context.flags.carry = 1;
				return false;
			}

			pcb->fd_table[0] = context.rbx.e >> 16;
			pcb->fd_table[1] = context.rbx.e & 0xFFFF;
			pcb->last_fd = 2;

			pcb->ppid = ppcb->pid;
			pcb->working_directory = ppcb->working_directory;
			ppcb->cpids.push_back(pid);
			
			process_table.emplace(pcb->pid, pcb);
			
			/*kiv_vfs::TPath tpath;
			tpath.mount = "proc";
			tpath.path = std::vector<std::string>{ pcb->name};
			tpath.file = pcb->pid;
			tpath.absolute_path = "proc:/" + pcb->pid + '/';*/

		}
		lock.unlock();

		kiv_thread::CThread_Manager::Get_Instance().Create_Thread(pid, context, func);

		return true;
	}

	bool CProcess_Manager::Get_Pcb(size_t tid, std::shared_ptr<TProcess_Control_Block> *pcb) {

		std::shared_ptr<kiv_thread::TThread_Control_Block> tcb;
		if (kiv_thread::CThread_Manager::Get_Instance().Get_Thread_Control_Block(tid, &tcb) == false) {
			return false;
		}
		else {
			*pcb = tcb->pcb;
			return true;
		}

	}

	//bool CProcess_Manager::Get_Tcb(size_t tid, std::shared_ptr<kiv_thread::TThread_Control_Block> tcb) {
	//	for (auto tpcb : process_table) {
	//		for (std::shared_ptr<kiv_thread::TThread_Control_Block> ttcb : tpcb.second->thread_table) {
	//			if (ttcb->tid == tid) {

	//				if (tcb != nullptr) {
	//					tcb = ttcb;
	//				}

	//				return true;
	//			}
	//		}
	//	}

	//	return false;

	//}

	//Zkontroluje zda process skoncil tzn. vsechny vlakna jsou ve stavu terminated
	//a provede potrebne akce
	void CProcess_Manager::Check_Process_State(std::shared_ptr<TProcess_Control_Block> pcb) {
		
		std::unique_lock<std::mutex> lock(ptable);
		{

			bool terminated = true;
			auto itr = pcb->thread_table.begin();
			while (itr != pcb->thread_table.end()) {

				//Pokud vlakno skoncilo tak ho smazeme
				if ((*itr)->state == kiv_thread::NThread_State::TERMINATED) {

					(*itr)->pcb = nullptr;
					(*itr)->thread.detach(); // musime pouzit join() nebo detach() predtim nez znicime objekt std::thread
					itr = pcb->thread_table.erase(itr);

				}
				else {
					terminated = false;
					itr++;
				}
			}

			//Pokud jiz nebezi zadne vlakno v procesu tzn. proces je ukoncen
			if (terminated) {
				
				//Uzavirani vsech otevrenych souboru
				for (auto &item : pcb->fd_table) {
					//TODO zavreni stdin a stdout?
					if (item.second != 0 && item.second != 1) {
						kiv_vfs::CVirtual_File_System::Get_Instance().Close_File(item.second);
					}
				}

				pcb->fd_table.clear();

				//Pokud bezi nejake child procesy tak je predame rodici
				for (size_t cpid : pcb->cpids) {
					if (process_table[cpid]->state != NProcess_State::TERMINATED) {
						process_table[pcb->ppid]->cpids.push_back(cpid);
						process_table[cpid]->ppid = pcb->ppid;
						//SIGNALIZE SYSTEM PROCESS
						reap_cv.notify_all();
					}
				}

				//
				std::vector<size_t> &vec = process_table[pcb->ppid]->cpids;
				vec.erase(std::remove(vec.begin(), vec.end(), pcb->pid), vec.end());

				process_table.erase(pcb->pid);
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
		reap_cv.notify_all();
		process_table[0]->thread_table[0]->thread.join();

		for (auto pcb : process_table) {

			for (auto tcb : pcb.second->thread_table) {

				//Systemove procesy nebudeme ukoncovat nasilne
				if (pcb.second->pid == 0) {		
					tcb->pcb = nullptr;
				}
				else if (tcb->terminate_handler == nullptr) {
					//NO TIME FOR MERCY, KILL IT!
					tcb->thread.detach();
					kiv_thread::Kiv_Os_Default_Terminate_Handler(tcb);
					tcb->pcb = nullptr;
				}
				else {
					tcb->terminate_handler(regs);
					// TODO mohlo by se stat ze se nedockam
					tcb->thread.join();
					tcb->pcb = nullptr;
				}

			}

			pcb.second->thread_table.clear();
		}
		
		process_table.clear();
		kiv_thread::CThread_Manager::Get_Instance().thread_map.clear();

		//stdin and stdout closing
		kiv_vfs::CVirtual_File_System::Get_Instance().Close_File(0);
		kiv_vfs::CVirtual_File_System::Get_Instance().Close_File(1);
	}

	bool CProcess_Manager::Set_Working_Directory(const kiv_vfs::TPath &dir) {


		std::shared_ptr<kiv_thread::TThread_Control_Block> tcb;
		if (kiv_thread::CThread_Manager::Get_Instance().Get_Thread_Control_Block(kiv_thread::Hash_Thread_Id(std::this_thread::get_id()), &tcb)) {
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

	bool CProcess_Manager::Get_Working_Directory(kiv_vfs::TPath * dir) const {

		std::shared_ptr<kiv_thread::TThread_Control_Block> tcb;

		if (kiv_thread::CThread_Manager::Get_Instance().Get_Thread_Control_Block(kiv_thread::Hash_Thread_Id(std::this_thread::get_id()), &tcb)) {
			*dir = tcb->pcb->working_directory;
		}
		else {
			return false;
		}

		return true;

	}

	kiv_os::THandle CProcess_Manager::Save_Fd(const std::shared_ptr<TProcess_Control_Block> &pcb, const kiv_os::THandle &fd_index) {

		std::unique_lock<std::mutex> lock(ptable);
		{
			pcb->fd_table.emplace(pcb->last_fd++, fd_index);
		}
		lock.unlock();

		return pcb->last_fd - 1;

	}

	kiv_os::THandle CProcess_Manager::Save_Fd(const kiv_os::THandle &fd_index) {

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

	bool CProcess_Manager::Get_Fd(const kiv_os::THandle &position, kiv_os::THandle &fd) {

		std::shared_ptr<kiv_thread::TThread_Control_Block> tcb;

		if (kiv_thread::CThread_Manager::Get_Instance().Get_Thread_Control_Block(kiv_thread::Hash_Thread_Id(std::this_thread::get_id()), &tcb)) {	
			auto result = tcb->pcb->fd_table.find(position);
			if (result == tcb->pcb->fd_table.end()) {
				return false;
			}
			else {
				fd = result->second;
				return true;
			}
		}
		else {
			return false;
		}
	}

	bool CProcess_Manager::Get_Name(const size_t pid, std::string & name) {
		return false;
	}

	std::map<size_t, std::string> CProcess_Manager::Get_Processes() {
		return std::map<size_t, std::string>();
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
			pcb->working_directory = kiv_vfs::DEFAULT_WORKING_DIRECTORY;

			// Nastavíme vlákno pro systémový proces
			std::shared_ptr<kiv_thread::TThread_Control_Block> tcb = std::make_shared<kiv_thread::TThread_Control_Block>();
			tcb->pcb = pcb;
			tcb->state = kiv_thread::NThread_State::RUNNING;
			tcb->terminate_handler = nullptr;
			
			std::unique_lock<std::mutex> tm_lock(kiv_thread::CThread_Manager::Get_Instance().maps_lock);
			{
				//CHECK pri nacitani dll se zasekne pokud ihned inicializujeme instanci
				tcb->thread = std::thread(&CProcess_Manager::Reap_Process, this);
			
				tcb->tid = kiv_thread::Hash_Thread_Id(std::this_thread::get_id());

				//std::shared_ptr<kiv_thread::TThread_Control_Block> ptr = tcb;
				kiv_thread::CThread_Manager::Get_Instance().thread_map.emplace(tcb->tid, tcb);
				kiv_thread::CThread_Manager::Get_Instance().thread_map.emplace(kiv_thread::Hash_Thread_Id(tcb->thread.get_id()), tcb);

				// Otevre stdin a stdout
			}
			tm_lock.unlock();

			kiv_os::THandle fd_index = 0;
			kiv_vfs::CVirtual_File_System::Get_Instance().Open_File("stdio:\\stdin", kiv_os::NFile_Attributes::System_File, fd_index);
			kiv_vfs::CVirtual_File_System::Get_Instance().Open_File("stdio:\\stdout", kiv_os::NFile_Attributes::System_File, fd_index);

			pcb->thread_table.push_back(tcb);
			process_table.emplace(pcb->pid, pcb);
		}
		lock.unlock();
	}

	//Stara se o procesy, kterym skoncil rodicovsky proces aniz by si precetl
	//navratovy kod
	void CProcess_Manager::Reap_Process() {

		std::vector<size_t> child_pids;
		std::vector<size_t> handles;

		while (!system_shutdown) {

			std::unique_lock<std::mutex> lock(reap_mutex);
			reap_cv.wait(lock);
			lock.unlock();

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
			//else {
			//	//Pokud nedostaneme zamek nad process_table nema cenu pokracovat, ale chceme se dostat k praci co nejdrive
			//	//TODO lock with semaphore
			//	std::this_thread::yield();
			//}
			

			uint16_t exit_code;
			for (const size_t handle : handles) {
				kiv_thread::CThread_Manager::Get_Instance().Read_Exit_Code(handle, exit_code);
			}

			/*std::this_thread::yield();*/
		}

	}

#pragma endregion

#pragma endregion

}