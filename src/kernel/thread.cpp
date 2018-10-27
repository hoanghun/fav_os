#include <memory>

#include "thread.h"
#include "process.h"
#include "common.h"
#include "..\api\api.h"

namespace kiv_thread {

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
				tcb->tid = tcb->thread.get_id();
				tcb->terminate_handler = nullptr;


				pcb->thread_table.push_back(tcb);
			}
			kiv_process::CProcess_Manager::ptable.unlock();

			//TODO REMOVE
			tcb->thread.join();

			return true;
		}

		//Vytvori vlakno pro jiz existujici proces
		bool  CThread_Manager::Create_Thread(kiv_hal::TRegisters& context) {

			std::shared_ptr<kiv_process::TProcess_Control_Block> pcb = std::make_shared<kiv_process::TProcess_Control_Block>();

			kiv_process::CProcess_Manager::ptable.lock();
			{

				if (!kiv_process::CProcess_Manager::Get_Instance().Get_Pcb(std::this_thread::get_id(), pcb)) {
					return false;
				}

			}
			kiv_process::CProcess_Manager::ptable.unlock();

			Create_Thread(pcb->pid, context);

			return false;
		}

		//Funkce je volana po skonceni vlakna/procesu
		bool CThread_Manager::Thread_Exit(kiv_hal::TRegisters& context) {

			std::shared_ptr<TThread_Control_Block> tcb = std::make_shared<TThread_Control_Block>();

			kiv_process::CProcess_Manager::ptable.lock();
			{
				if (!kiv_process::CProcess_Manager::Get_Instance().Get_Tcb(std::this_thread::get_id(), tcb)) {
					return false;
				}
			
				tcb->state = NThread_State::TERMINATED;
				
			}
			kiv_process::CProcess_Manager::ptable.unlock();

			kiv_process::CProcess_Manager::Get_Instance().Check_Process_State(tcb->pcb->pid);

			return true;
		}

		// Prida vlaknu/procesu hendler na funkci, ktera ho ukonci
		bool CThread_Manager::Add_Terminate_Handler(const kiv_hal::TRegisters& context) {

			std::shared_ptr<TThread_Control_Block> tcb = std::make_shared<TThread_Control_Block>();

			kiv_process::CProcess_Manager::ptable.lock(); 
			{
				if (!kiv_process::CProcess_Manager::Get_Instance().Get_Tcb(std::this_thread::get_id(), tcb)) {
					return false;
				}
			}
			kiv_process::CProcess_Manager::ptable.unlock();

			// Pokud je rdx.r == 0 potom se ulozi do terminat_handler (stejne uz by tam mela byt)
			tcb->terminate_handler = reinterpret_cast<kiv_os::TThread_Proc>(context.rdx.r); 

			return true;

		}

}