#pragma once

#include <vector>
#include <thread>

#include "..\api\api.h"

namespace kiv_process {
		struct TProcess_Control_Block;
}

namespace kiv_thread {
		enum NThread_State {
			RUNNING = 1,
			BLOCKED,
			TERMINATED
		};

		struct TThread_Control_Block {
			std::thread::id tid;
			std::thread thread;
			std::shared_ptr<kiv_process::TProcess_Control_Block> pcb;
			NThread_State state;
			kiv_os::TThread_Proc terminate_handler;
			uint16_t exit_code;
		};

		inline void Kiv_Os_Default_Terminate_Handler(std::shared_ptr<TThread_Control_Block> tcb);

		class CThread_Manager {

		public:

			static CThread_Manager & Get_Instance();
			static void Destroy();

			bool Create_Thread(size_t pid, kiv_hal::TRegisters& context);
			bool Create_Thread(kiv_hal::TRegisters& context);

			bool Exit_Thread(kiv_hal::TRegisters& context);
			bool Add_Terminate_Handler(const kiv_hal::TRegisters& context);

		private:

			static CThread_Manager * instance;

			CThread_Manager();
		};
}
