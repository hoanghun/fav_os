#include <memory>

#include "thread.h"
#include "process.h"
#include "common.h"
#include "..\api\api.h"

namespace kiv_thread {

#pragma endregion
		
		CThread_Manager * CThread_Manager::instance = NULL;

		CThread_Manager::CThread_Manager() {
		}

		void CThread_Manager::Destroy() {
			delete instance;
		}

		CThread_Manager & CThread_Manager::Get_Instance() {

			if (instance == NULL) {
				instance = new CThread_Manager();
			}

			return *instance;

		}

		// Vyvoti vlakno pro proces
		bool CThread_Manager::Create_Thread(const size_t pid, kiv_hal::TRegisters& context) {

			const char* func_name = (char*)(context.rdx.r);

			std::shared_ptr<kiv_process::TProcess_Control_Block> pcb = kiv_process::CProcess_Manager::Get_Instance().process_table[pid];

			kiv_os::TThread_Proc func = (kiv_os::TThread_Proc) GetProcAddress(User_Programs, func_name);

			if (!func) {
				return false;
			}
	
			//TODO move to block ??
			std::shared_ptr<TThread_Control_Block> tcb = std::make_shared<TThread_Control_Block>();

			// uzamknuti tabulky procesu tzn. i tabulky vlaken
			// mozna by slo udelat efektivneji nez zamykat celou tabulku
			kiv_process::CProcess_Manager::ptable.lock();
			{
				//std::shared_ptr<TThread_Control_Block> tcb = std::make_shared<TThread_Control_Block>();

				tcb->thread = std::thread(func, context);

				tcb->pcb = pcb;
				tcb->state = NThread_State::RUNNING;
				tcb->tid = Hash_Thread_Id(tcb->thread.get_id());

				//return handle to parent process
				context.rax.r = tcb->tid;

				std::unique_lock<std::mutex> lock(maps_lock);
				{
					std::shared_ptr<TThread_Control_Block> ptr = tcb;
					thread_map.emplace(tcb->tid, tcb);
				}
				lock.unlock();



				tcb->terminate_handler = nullptr;


				pcb->thread_table.push_back(tcb);
			}
			kiv_process::CProcess_Manager::ptable.unlock();

			//TODO REMOVE
			//tcb->thread.join();

			return true;
		}

		//Vytvori vlakno pro jiz existujici proces
		bool  CThread_Manager::Create_Thread(kiv_hal::TRegisters& context) {

			std::shared_ptr<kiv_process::TProcess_Control_Block> pcb;

			std::unique_lock<std::mutex> lock(maps_lock);
			{
				auto result = thread_map.find(Hash_Thread_Id(std::this_thread::get_id()));
				if (result == thread_map.end()) {
					lock.unlock();
					return false;
				}
				else {
					pcb = result->second->pcb;
				}
			}
			lock.unlock();

			Create_Thread(pcb->pid, context);

			return false;
		}

		//Funkce je volana po skonceni vlakna/procesu
		bool CThread_Manager::Thread_Exit(kiv_hal::TRegisters& context) {

			std::shared_ptr<TThread_Control_Block> tcb;
			std::unique_lock<std::mutex> plock(kiv_process::CProcess_Manager::ptable);
			{

				std::unique_lock<std::mutex> lock(maps_lock);
				{
					auto result = thread_map.find(Hash_Thread_Id(std::this_thread::get_id()));
					if (result == thread_map.end()) {
						plock.unlock();
						lock.unlock();
						return false;
					}
					else {
						tcb = result->second;
						tcb->state = NThread_State::TERMINATED;
					}
				}
				lock.unlock();

				//Signalizace ukonceni procesu tem kdo na to cekaji
				for (auto const & e : tcb->waiting) {
					*e = true;
				}
				tcb->waiting.clear();

			}
			plock.unlock();

			return true;
		}

		// Prida vlaknu/procesu hendler na funkci, ktera ho ukonci
		bool CThread_Manager::Add_Terminate_Handler(const kiv_hal::TRegisters& context) {

			std::shared_ptr<TThread_Control_Block> tcb;

			std::unique_lock<std::mutex> lock(maps_lock);
			{
				auto result = thread_map.find(Hash_Thread_Id(std::this_thread::get_id()));
				if (result == thread_map.end()) {
					lock.unlock();
					return false;
				}
				else {
					tcb = result->second;
				}
			}
			lock.unlock();

			// Pokud je rdx.r == 0 potom se ulozi do terminat_handler (stejne uz by tam mela byt)
			tcb->terminate_handler = reinterpret_cast<kiv_os::TThread_Proc>(context.rdx.r); 

			return true;

		}

		void CThread_Manager::Wait_For(kiv_hal::TRegisters& context) {

			const size_t * tids = reinterpret_cast<size_t *>(context.rdx.r);
			const size_t tids_count = context.rcx.r;

			std::unique_lock<std::mutex> lock(maps_lock);
			{
				for (int i = 0; i < tids_count; i++) {

					auto result = thread_map.find(tids[i]);

					if (result == thread_map.end()) {
						//TODO raise some error??
						context.rax.r = -1;
						lock.unlock();
						return;
					}
					else if (result->second->state == NThread_State::TERMINATED) {
						context.rax.r = tids[i];
						lock.unlock();
						return;
					}
				}
			}
			lock.unlock();

			int index = Wait(tids, tids_count);
			context.rax.r = tids[index]; 

		}

		int CThread_Manager::Wait(const size_t * tids, const size_t tids_count) {

			bool * events = new bool[tids_count];
			for (int i = 0; i < tids_count; i++) {
				events[i] = false;
			}

			for (int i = 0; i < tids_count; i++) {
				/*std::shared_ptr<bool> sptr = std::make_shared<bool>(events[i]);*/
				Add_Event(tids[i], &events[i]);
				events[i] = false;
				i++;
			}

			for (;;) {
				for (int i = 0; i < tids_count; i++) {

					if (events[i]) {
						delete events;
						return i;
					}
				}

				std::this_thread::yield();
			}

		}

		void CThread_Manager::Add_Event(const size_t tid, bool * e) {
			
			std::shared_ptr<TThread_Control_Block> tcb;

			std::unique_lock<std::mutex> m_lock(maps_lock);
			{
				auto result = thread_map.find(tid);

				if (result == thread_map.end()) {
					m_lock.unlock();
					return;
				}
				else {
					tcb = result->second;
				}
			}
			m_lock.unlock();

			std::unique_lock<std::mutex> e_lock(tcb->waiting_lock);
			{
				tcb->waiting.push_back(e);
			}
			e_lock.unlock();

		}

		bool CThread_Manager::Read_Exit_Code(kiv_hal::TRegisters &context) {

			return Read_Exit_Code(context.rdx.r, context.rcx.x);

		}

		bool CThread_Manager::Read_Exit_Code(const size_t handle, uint16_t &exit_code) {

			std::shared_ptr<TThread_Control_Block> tcb;
			NThread_State terminated = NThread_State::RUNNING;

			std::unique_lock<std::mutex> lock(maps_lock);
			{
				auto result = thread_map.find(handle);
				if (result == thread_map.end()) {
					lock.unlock();
					return false;
				}
				else {
					tcb = result->second;
					//return exit_code
					terminated = tcb->state;
					if (terminated == NThread_State::TERMINATED) {
						exit_code = tcb->exit_code;
						thread_map.erase(tcb->tid);
					}
					else {
						lock.unlock();
						return false;
					}

				}
			}
			lock.unlock();

			if (terminated == NThread_State::TERMINATED) {
				kiv_process::CProcess_Manager::Get_Instance().Check_Process_State(tcb->pcb);
			}

			return true;

		}

}