#pragma once

#include <vector>
#include <array>
#include <string>
#include <memory>
#include <mutex>
#include <map>

#include "thread.h"
#include "../api/hal.h"
#include "vfs.h"

namespace kiv_process {
		void Handle_Process(kiv_hal::TRegisters &regs);

		const int PID_NOT_AVAILABLE = -1;
		
		class CPid_Manager {
			public:
				CPid_Manager();

				static const size_t MAX_PROCESS_COUNT = 1024;
				bool Get_Free_Pid(size_t* pid);
				bool Release_Pid(size_t pid);

			private:
				std::mutex plock;
				std::array<bool, MAX_PROCESS_COUNT> pids{false};
				size_t last = 0;
				bool is_full = false;
		};
		
		enum NProcess_State {
			RUNNING = 1,
			BLOCKED,
			TERMINATED
		};

		struct TProcess_Control_Block {
			std::string name;
			size_t pid;
			size_t ppid;
			std::vector<size_t> cpids;
			NProcess_State state;
			kiv_vfs::TPath working_directory;

			std::vector<std::shared_ptr<kiv_thread::TThread_Control_Block>> thread_table;
			std::map<kiv_os::THandle, kiv_os::THandle> fd_table; // Process handle -> VFS handle
			kiv_os::THandle last_fd;
		};

		class CProcess_Manager {

			friend class kiv_thread::CThread_Manager;

			public:
				static CProcess_Manager &Get_Instance();
				static void Destroy();

				bool Create_Process(kiv_hal::TRegisters& context);
				//bool Exit_Process(kiv_hal::TRegisters& context);

				bool Set_Working_Directory(const kiv_vfs::TPath &dir);
				bool Get_Working_Directory(kiv_vfs::TPath *dir) const;
				
				kiv_os::THandle Save_Fd(const kiv_os::THandle &fd_index);
				bool Remove_Fd(const kiv_os::THandle &fd_index);
				bool Get_Fd(const kiv_os::THandle &position, kiv_os::THandle &fd);


				/*
				*/
				bool Get_Name(const size_t pid, std::string &name);

				std::map<size_t, std::string> Get_Processes();


				void Shutdown();
				void Execute_Shutdown();

			private:
				static std::mutex ptable;
				static CProcess_Manager *instance;
				CPid_Manager *pid_manager;
				std::map<size_t, std::shared_ptr<TProcess_Control_Block>> process_table;

				CProcess_Manager();
				bool Get_Pcb(size_t tid, std::shared_ptr<TProcess_Control_Block> *ppcb);
				//bool Get_Tcb(size_t tid, std::shared_ptr<kiv_thread::TThread_Control_Block> tcb);
				void Check_Process_State(std::shared_ptr<TProcess_Control_Block> pid);
				void Create_Sys_Process();
				void Reap_Process();

				kiv_os::THandle Save_Fd(const std::shared_ptr<TProcess_Control_Block> &pcb, const kiv_os::THandle &fd_index);
				void Remove_Fd(const std::shared_ptr<TProcess_Control_Block> &pcb, const kiv_os::THandle &fd_index);

		};
}

