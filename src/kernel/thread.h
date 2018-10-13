#pragma once

#include <vector>
#include <thread>
#include "..\api\api.h"

namespace kiv_process {

		struct TProcess_Control_Block;

}

namespace kiv_thread {

		enum NThread_State {

			RUNNING,
			BLOCKED,
			TERMINATED

		};

		struct TThread_Control_Block {

			std::thread::id tid;
			std::shared_ptr<kiv_process::TProcess_Control_Block> pcb;
			NThread_State state;

		};


		class CThread_Manager {

		public:

			static CThread_Manager & Get_Instance();
			~CThread_Manager();

			bool Create_Thread(const size_t pid, const char* func_name, const kiv_hal::TRegisters& context);
			bool Create_Thread(const kiv_hal::TRegisters& context);

			bool Exit_Thread();

			

		private:

			static CThread_Manager * instance;

			CThread_Manager();
	
		};
}
