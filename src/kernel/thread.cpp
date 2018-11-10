#include <memory>

#include "thread.h"
#include "process.h"
#include "common.h"
#include "..\api\api.h"
#include "kernel.h"

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
			kiv_hal::TRegisters stdin_regs;
			kiv_hal::TRegisters stdout_regs;

			// move to different file ! TODO
			stdin_regs.rax.h = static_cast<uint8_t>(kiv_os::NOS_Service_Major::File_System);
			stdin_regs.rax.l = static_cast<uint8_t>(kiv_os::NOS_File_System::Open_File);
			stdin_regs.rdx.r = reinterpret_cast<decltype(stdin_regs.rdx.r)>(kiv_vfs::STDIN_PATH.c_str());
			stdin_regs.rcx.r = static_cast<uint8_t>(kiv_os::NOpen_File::fmOpen_Always);
			stdin_regs.rdi.r = static_cast<uint8_t>(kiv_os::NFile_Attributes::System_File);

			Sys_Call(stdin_regs);

			stdout_regs.rax.h = static_cast<uint8_t>(kiv_os::NOS_Service_Major::File_System);
			stdout_regs.rax.l = static_cast<uint8_t>(kiv_os::NOS_File_System::Open_File);
			stdout_regs.rdx.r = reinterpret_cast<decltype(stdin_regs.rdx.r)>(kiv_vfs::STDOUT_PATH.c_str());
			stdout_regs.rcx.r = static_cast<uint8_t>(kiv_os::NOpen_File::fmOpen_Always);
			stdout_regs.rdi.r = static_cast<uint8_t>(kiv_os::NFile_Attributes::System_File);

			Sys_Call(stdout_regs);

			context.rax.x = stdin_regs.rax.x;
			context.rbx.x = stdout_regs.rax.x;

			kiv_process::CProcess_Manager::Get_Instance().Save_Fd(context.rax.x);
			kiv_process::CProcess_Manager::Get_Instance().Save_Fd(context.rbx.x);

			std::shared_ptr<TThread_Control_Block> tcb = std::make_shared<TThread_Control_Block>();

			// uzamknuti tabulky procesu tzn. i tabulky vlaken
			// mozna by slo udelat efektivneji nez zamykat celou tabulku
			std::unique_lock<std::mutex> lock(kiv_process::CProcess_Manager::ptable);
			{

				tcb->thread = std::thread(func, context);

				tcb->pcb = pcb;
				tcb->state = NThread_State::RUNNING;
				tcb->tid = Hash_Thread_Id(tcb->thread.get_id());

				//return handle to parent process
				context.rax.r = tcb->tid;

				std::unique_lock<std::mutex> tm_lock(maps_lock);
				{
					std::shared_ptr<TThread_Control_Block> ptr = tcb;
					thread_map.emplace(tcb->tid, tcb);
				}
				tm_lock.unlock();



				tcb->terminate_handler = nullptr;


				pcb->thread_table.push_back(tcb);
			}
			lock.unlock();

			return true;
		}

		//Vytvori vlakno pro jiz existujici proces
		bool  CThread_Manager::Create_Thread(kiv_hal::TRegisters& context) {

			std::shared_ptr<TThread_Control_Block> tcb;

			if (Get_Thread_Control_Block(Hash_Thread_Id(std::this_thread::get_id()), &tcb)) {
				Create_Thread(tcb->pcb->pid, context);
			}
			else {
				return false;
			}

			return true;
		}

		//Funkce je volana po skonceni vlakna/procesu
		bool CThread_Manager::Thread_Exit(kiv_hal::TRegisters& context) {

			std::shared_ptr<TThread_Control_Block> tcb;
			std::unique_lock<std::mutex> plock(kiv_process::CProcess_Manager::ptable);
			{

				if (Get_Thread_Control_Block(Hash_Thread_Id(std::this_thread::get_id()), &tcb)) {
					tcb->state = NThread_State::TERMINATED;
				}
				else {
					plock.unlock();
					return false;
				}

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

			if (Get_Thread_Control_Block(Hash_Thread_Id(std::this_thread::get_id()), &tcb) == false) {
				return false;
			}

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

			if (Get_Thread_Control_Block(tid, &tcb) == false) {
				return;
			}

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

			if (Get_Thread_Control_Block(handle, &tcb)) {
				std::unique_lock<std::mutex> lock(maps_lock);
				{
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
				lock.unlock();
			}
			else {
				return false;
			}

			if (terminated == NThread_State::TERMINATED) {
				kiv_process::CProcess_Manager::Get_Instance().Check_Process_State(tcb->pcb);
			}

			return true;

		}

		bool CThread_Manager::Get_Thread_Control_Block(const size_t &tid, std::shared_ptr<TThread_Control_Block> *tcb) {

			std::unique_lock<std::mutex> lock(maps_lock);
			{
				auto result = thread_map.find(tid);
				if (result == thread_map.end()) {
					lock.unlock();
					return false;
				}
				else {
					*tcb = result->second;
				}
			}
			lock.unlock();

			return true;
		}

}