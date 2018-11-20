#pragma once

#include <mutex>
#include <condition_variable>
#include <vector>
#include <map>
#include <thread>
#include <Windows.h>

#include "semaphore.h"
#include "..\api\api.h"

namespace kiv_process {
		struct TProcess_Control_Block;
		class CProcess_Manager;
}

namespace kiv_thread {

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
			Semaphore *wait_semaphore;
			std::mutex waiting_lock;
			std::vector<size_t> waiting_threads;
		};
			

		inline size_t Hash_Thread_Id(const std::thread::id tid) {
			return std::hash<std::thread::id>{}(tid);
		}

		inline void Kiv_Os_Default_Terminate_Handler(std::shared_ptr<TThread_Control_Block> tcb) {
			//TODO change -1 to some exit code
			TerminateThread(tcb->thread.native_handle(), -1);
		}

		class CThread_Manager {

			friend class kiv_process::CProcess_Manager;

			public:

				static CThread_Manager & Get_Instance();
				static void Destroy();

				bool Create_Thread(const size_t pid, kiv_hal::TRegisters& context, kiv_os::TThread_Proc &func);
				bool Create_Thread(kiv_hal::TRegisters& context);

				bool Thread_Exit(kiv_hal::TRegisters& context);
				bool Add_Terminate_Handler(kiv_hal::TRegisters& context);
			

				void Wait_For(kiv_hal::TRegisters& context);
				bool Add_Event(const size_t tid, const size_t my_tid);
				bool Check_Event(const size_t tid, const size_t my_tid);
				bool Read_Exit_Code(kiv_hal::TRegisters &context);
				bool Read_Exit_Code(const size_t handle, uint16_t &exit_code);

			private:

				std::map<size_t, std::shared_ptr<TThread_Control_Block>> thread_map;
				std::mutex maps_lock;
		
				size_t Wait(const size_t * tids, const size_t tids_count);
				bool Get_Thread_Control_Block(const size_t &tid, std::shared_ptr<TThread_Control_Block> *tcb);

				static CThread_Manager * instance;
				CThread_Manager();
		
		};
}
