#include <cstring>

#include "process.h"

namespace kiv_process {

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
		}

		std::shared_ptr<TProcess_Control_Block> pcb = std::make_shared<TProcess_Control_Block>();

		pcb->pid = pid;
		pcb->state = NProcess_State::RUNNING;
		// TODO name

		size_t ppid;
		std::shared_ptr<TProcess_Control_Block> ppcb;

		if (!Get_Pid(std::this_thread::get_id(), &ppid, ppcb)) {
			//TODO error
		}

		pcb->ppid = ppid;
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

	bool CProcess_Manager::Get_Pid(std::thread::id tid, size_t* pid, std::shared_ptr<TProcess_Control_Block> pcb) {

		for(std::shared_ptr<TProcess_Control_Block> tpcb : process_table) {

			for (std::shared_ptr<kiv_thread::TThread_Control_Block> ttcb : pcb->thread_table) {
				if (ttcb->tid == tid) {
					*pid = tpcb->pid;
					pcb = tpcb;
					return true;
				}
			}

		}

		return false;

	}

#pragma endregion

}