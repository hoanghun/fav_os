#pragma once

#include <vector>
#include <array>
#include <string>
#include <memory>

#include "thread.h"
#include "../api/hal.h"

namespace kiv_process {
	
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
			RUNNING,
			BLOCKED,
			TERMINATED
		};

		struct TProcess_Control_Block {

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

				static CProcess_Manager & Get_Instance();
				virtual ~CProcess_Manager();

				bool Create_Process(kiv_hal::TRegisters& context);
				bool Exit_Process(kiv_hal::TRegisters& context);

			private:

				static CProcess_Manager * instance;
				CPid_Manager pid_manager;
				std::vector<std::shared_ptr<TProcess_Control_Block>> process_table;

				CProcess_Manager();
				bool Get_Pid(std::thread::id tid, size_t* pid, std::shared_ptr<TProcess_Control_Block> pcb);
		};
}

