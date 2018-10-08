#pragma once

#include <vector>

namespace kiv_os {
	namespace process {

		struct TProcess_Control_Block;

	}
}

namespace kiv_os {
	namespace thread {

		enum NThread_State {

			RUNNING,
			BLOCKED,
			TERMINATED

		};

		struct TThread_Control_Block {

			unsigned int tid;
			kiv_os::process::TProcess_Control_Block & pcb;
			NThread_State state;

		};


		class CThread_Manager {

		public:

			static CThread_Manager & Get_Instance();
			~CThread_Manager();

			bool Create_Thread();
			bool Exit_Thread();

			

		private:

			static CThread_Manager * instance;

			CThread_Manager();

			std::vector<kiv_os::process::TProcess_Control_Block> & Get_Process_Control_Block(int tid);
	
		};
	}
}
