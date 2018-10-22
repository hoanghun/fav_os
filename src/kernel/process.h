#pragma once

#include <vector>
#include <array>
#include <string>
#include <memory>
#include <mutex>

#include "thread.h"
#include "../api/hal.h"
#include "source_manager.h"

namespace kiv_process {

		void Handle_Process(kiv_hal::TRegisters &regs);

		const int PID_NOT_AVAILABLE = -1;
		
		class CPid_Manager {
			public:
				static const size_t MAX_PROCESS_COUNT = 1024;
				bool Get_Free_Pid(size_t* pid);
				bool Release_Pid(size_t pid);

			private:
				std::array<bool, MAX_PROCESS_COUNT> pids{false};
				size_t last = 0;
				bool is_full = false;
		};
		
		
		enum NProcess_State {
			RUNNING = 1,
			BLOCKED,
			TERMINATED
		};

		struct TControl_Block {
			kiv_os::THandle owner;
		};

		struct TProcess_Control_Block : public TControl_Block {
			std::string name;
			size_t pid;
			size_t ppid;
			std::vector<size_t> cpids;
			NProcess_State state;

			std::vector<std::shared_ptr<kiv_thread::TThread_Control_Block>> thread_table;

			//file deskriptory
			//working dir
		};

		class CProcess_Manager {

			friend class kiv_thread::CThread_Manager;

			public:
				static CProcess_Manager &Get_Instance();
				static void Destroy();

				bool Create_Process(kiv_hal::TRegisters& context);
				//bool Exit_Process(kiv_hal::TRegisters& context);
				void Shutdown();

			private:
				static std::mutex ptable;
				static CProcess_Manager *instance;
				CPid_Manager pid_manager;
				std::vector<std::shared_ptr<TProcess_Control_Block>> process_table;

				CProcess_Manager();
				bool Get_Pcb(std::thread::id tid, std::shared_ptr<TProcess_Control_Block> pcb);
				bool Get_Tcb(std::thread::id tid, std::shared_ptr<kiv_thread::TThread_Control_Block> tcb);
				void Check_Process_State(size_t pid);
				void Create_Sys_Process();
		};
}

