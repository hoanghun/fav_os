#include "shell.h"
#include "rtl.h"
#include "stdio.h"
#include "parser.h"

#include <sstream>
#include <map>
#include <algorithm>
#include <iostream>

bool shell_run = true;

size_t __stdcall shell_terminate_handler(const kiv_hal::TRegisters &regs) {
	shell_run = false;
	return 0;
}

size_t __stdcall shell(const kiv_hal::TRegisters &regs) {

	kiv_os_rtl::Register_Terminate_Signal_Handler(shell_terminate_handler);

	const size_t buffer_size = 256;
	char buffer[buffer_size];
	size_t counter;
	
	const kiv_os::THandle sin = regs.rax.x;
	const kiv_os::THandle sout = regs.rbx.x;

	const char* intro = "FAV Virtual OS [Version 1.0]\n" \
						"(c) 2018 FAVaci Corporation. All rights reserverd\n";
	
	kiv_os_rtl::Stdout_Print(regs, intro, strlen(intro));

	const char* new_line = "\n";
	const char prompt_char = '>';

	const size_t prompt_size = 512;
	char prompt[prompt_size];
	size_t prompt_read_count;

	do {
		kiv_os_rtl::Get_Working_Dir(prompt, prompt_size, prompt_read_count);
		prompt[prompt_read_count] = prompt_char;
		kiv_os_rtl::Stdout_Print(regs, prompt, prompt_read_count + 1);

		counter = kiv_os_rtl::Stdin_Read(regs, buffer, buffer_size);

		if (counter > 0) {
			if (counter == buffer_size) counter--;
			buffer[counter] = 0;	//udelame z precteneho vstup null-terminated retezec

			if (strcmp(buffer, "exit") == 0) {
				break;
			}

			std::vector<TExecutable> items = Parse(buffer, strlen(buffer));
			if (Check(items) == false) {
				const char *error = "\nCommand is not valid.";
				kiv_os_rtl::Stdout_Print(regs, error, strlen(error));
			}
			else {
				kiv_os_rtl::Stdout_Print(regs, new_line, strlen(new_line));
				Execute(items, regs);
			}
		}
		else
			break;	//EOF

		kiv_os_rtl::Stdout_Print(regs, new_line, strlen(new_line));
	} while (strcmp(buffer, "exit") != 0 && strcmp(buffer, "shutdown") != 0);

	kiv_os_rtl::Exit(0);

	return 0;
}

bool Check(std::vector<TExecutable> &exes) {

	//if command is cd check validity

	//kontrola zda jsou exes validni
	bool previous_pipe = false;
	for (const TExecutable &exe : exes) {
		if (exe.Check() == false) {
			return false;
		}

		if (exe.pipe_in == true && previous_pipe == false) {
			return false;
		}

		previous_pipe = exe.pipe_out;
	}
	
	if (exes.back().pipe_out == true || exes.front().pipe_in == true) {
		false;
	}

	return true;
}

//Pripravime soubory a pipy
void Prepare_For_Execution(TExecutable &exe, const kiv_os::THandle sin, const kiv_os::THandle sout, kiv_os::THandle &last_pipe) {
	if (exe.file_in.empty() == false) {
		kiv_os_rtl::Open_File(exe.file_in.c_str(), kiv_os::NOpen_File::fmOpen_Always, kiv_os::NFile_Attributes::Read_Only, exe.in_handle);
	}
	else if (exe.pipe_in) {
		exe.in_handle = last_pipe;
	}
	else {
		exe.in_handle = sin;
	}

	if (exe.file_out.empty() == false) {
		kiv_os_rtl::Open_File(exe.file_out.c_str(), kiv_os::NOpen_File(0), static_cast<kiv_os::NFile_Attributes>(0), exe.out_handle);
	}
	else if (exe.pipe_out) {
		//This pipe will be stdin for next process
		kiv_os_rtl::Create_Pipe(last_pipe, exe.out_handle);
	}
	else {
		exe.out_handle = sout;
	}
}

void Execute(std::vector<TExecutable> &exes, const kiv_hal::TRegisters &regs) {

	const kiv_os::THandle sin = regs.rax.x;
	const kiv_os::THandle sout = regs.rbx.x;

	std::vector<size_t> handles;
	size_t handle = 0;
	bool result = true;

	//cd
	if (exes.front().name == "cd") {
		Cd(exes.front(), regs);
		return;
	}

	kiv_os::THandle last_pipe = 0;

	for (TExecutable &exe : exes) {

		
			//Pripravime argumenty programu
			std::stringstream args;
			args.str("");
			if (exe.args.empty() == false) {
				args << exe.args[0];
			}
		
			for (int i = 1; i < exe.args.size(); i++) {
				args << ' ' << exe.args[i];
			}

			Prepare_For_Execution(exe, sin, sout, last_pipe);
			result = kiv_os_rtl::Clone(exe.name.c_str(), args.str().c_str(), exe.in_handle, exe.out_handle, handle);

			if (result == false) {
				switch (kiv_os_rtl::Last_Error) {
				case kiv_os::NOS_Error::Invalid_Argument:
				{
					const std::string error = "\n'" + exe.name + "' is not recognized as an internal or external command, operable program or batch file.";
					kiv_os_rtl::Stdout_Print(regs, error.c_str(), error.length());
					break;
				}
				default:
				{
					const std::string error = "\n Unknown error.";
					kiv_os_rtl::Stdout_Print(regs, error.c_str(), error.length());
					break;
				}
				}

				break;
			}
			else {
				handles.push_back(handle);
			}

		
	}

	size_t signaled;
	int exit_code;

	if (handles.size() != 0) {
		do {
			kiv_os_rtl::Wait_For(&handles[0], handles.size(), signaled);
			//TODO kontrolovat chyby
			handles.erase(std::remove(handles.begin(), handles.end(), signaled), handles.end());
			kiv_os_rtl::Read_Exit_Code(signaled, exit_code);

		} while (handles.size() > 0);
	}

}

void Cd(const TExecutable &exe, const kiv_hal::TRegisters &regs) {
	bool result = true;
	std::string err_msg;

	if (exe.args.size() != 1) {
		result = false;
		err_msg = "Wrong number of arguments.";
	}
	else {
		if (!kiv_os_rtl::Set_Working_Dir(exe.args.front().c_str())) {
			result = false;
			switch (kiv_os_rtl::Last_Error) {
				case kiv_os::NOS_Error::File_Not_Found:
					err_msg = "Directory does not exist.";
					break;
				case kiv_os::NOS_Error::Unknown_Error:
					err_msg = "Couldn't perform. Try again.";
					break;
			}
		}
	}

	if (!result) {
		kiv_os_rtl::Stdout_Print(regs, err_msg.c_str(), err_msg.size());
	}
}

