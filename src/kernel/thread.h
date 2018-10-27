#pragma once

#include <mutex>
#include <vector>
#include <map>
#include <thread>
#include <Windows.h>

#include "..\api\api.h"

namespace kiv_process {
		struct TProcess_Control_Block;
}

namespace kiv_thread {
		
		void Wait_For_Multiple(std::vector<bool> &events);

		enum NThread_State {
			RUNNING = 1,
			BLOCKED,
			TERMINATED
		};


		struct TThread_Control_Block {
			size_t tid;
			std::thread thread;
			std::shared_ptr<kiv_process::TProcess_Control_Block> pcb;
			NThread_State state;
			kiv_os::TThread_Proc terminate_handler;
			uint16_t exit_code;
			//Wait for
			std::mutex waiting_lock;
			std::vector<std::shared_ptr<bool>> waiting;
		};
			

		inline size_t Hash_Thread_Id(const std::thread::id tid) {
			return std::hash<std::thread::id>{}(tid);
		}

		inline void Kiv_Os_Default_Terminate_Handler(std::shared_ptr<TThread_Control_Block> tcb) {
			//TODO change -1 to some exit code
			TerminateThread(tcb->thread.native_handle(), -1);
		}

		class CThread_Manager {

		public:

			static CThread_Manager & Get_Instance();
			static void Destroy();

			bool Create_Thread(size_t pid, kiv_hal::TRegisters& context);
			bool Create_Thread(kiv_hal::TRegisters& context);

			bool Thread_Exit(kiv_hal::TRegisters& context);
			bool Add_Terminate_Handler(const kiv_hal::TRegisters& context);
			

			void Wait_For(kiv_hal::TRegisters& context);
			void Add_Event(const size_t tid, const std::shared_ptr<bool> e);

		private:

			std::map<size_t, std::shared_ptr<TThread_Control_Block>> thread_map;
			std::mutex maps_lock;
		
			static CThread_Manager * instance;
			CThread_Manager();
		
		};
}
