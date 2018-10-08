#pragma once

#include <vector>
#include <string>

#include "thread.h"

namespace kiv_os {
	namespace process {

		enum NProcess_State {
			RUNNING,
			BLOCKED,
			TERMINATED
		};

		struct TProcess_Control_Block {

			std::string name;
			unsigned int pid;
			unsigned int ppid;
			std::vector<unsigned int> cpids;
			NProcess_State state;

			//file deskriptory
			//working dir
		};

		class CProcess_Manager {

			friend class kiv_os::thread::CThread_Manager;

			public:

				static CProcess_Manager & Get_Instance();
				virtual ~CProcess_Manager();

				bool Create_Process();
				bool Exit_Process();

			private:

				static CProcess_Manager * instance;

				CProcess_Manager();
				std::vector<TProcess_Control_Block> Process_Table;

			

		};
	}
}

