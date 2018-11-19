#include "shell.h"
#include "rtl.h"
#include "stdio.h"
#include "parser.h"

#include <sstream>
#include <map>

size_t __stdcall shell(const kiv_hal::TRegisters &regs) {
	const size_t buffer_size = 256;
	char buffer[buffer_size];
	size_t counter;
	
	const kiv_os::THandle sin = regs.rax.x;
	const kiv_os::THandle sout = regs.rbx.x;

	const char* intro = "Vitejte v kostre semestralni prace z KIV/OS.\n" \
						"Shell zobrazuje echo zadaneho retezce. Prikaz exit ukonci shell.\n";
	

	kiv_os_rtl::Print_Line(regs, intro, strlen(intro));
	const char* prompt = "C:\\>";
	do {
		kiv_os_rtl::Print_Line(regs, prompt, strlen(prompt));

		counter = kiv_os_rtl::Read_Line(regs, buffer, buffer_size);
		if (counter > 0) {
			if (counter == buffer_size) counter--;
			buffer[counter] = 0;	//udelame z precteneho vstup null-terminated retezec

			std::vector<executable> items = Parse(buffer, strlen(buffer));
			Prepare_For_Execution(items, sin, sout);
			Execute(items);

			const char* new_line = "\n";
			kiv_os_rtl::Print_Line(regs, new_line, strlen(new_line));
			kiv_os_rtl::Print_Line(regs, buffer, strlen(buffer));
			kiv_os_rtl::Print_Line(regs, new_line, strlen(new_line));
		}
		else
			break;	//EOF
	} while (strcmp(buffer, "exit") != 0);


	kiv_os_rtl::Exit(0);


	return 0;
}

//Pripravime soubory a pipy
void Prepare_For_Execution(std::vector<executable> &exes, const kiv_os::THandle sin, const kiv_os::THandle sout) {

	//TODO zkontrolovat zda jsou exes validni

	kiv_os::THandle last_pipe = 0;
	for (executable &exe : exes) {
	
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
			if (exe.file_out_rewrite) {
				kiv_os_rtl::Open_File(exe.file_in.c_str(), static_cast<kiv_os::NOpen_File>(0), static_cast<kiv_os::NFile_Attributes>(0), exe.in_handle);
			}
			else {
				kiv_os_rtl::Open_File(exe.file_in.c_str(), kiv_os::NOpen_File::fmOpen_Always, static_cast<kiv_os::NFile_Attributes>(0), exe.in_handle);
			}
		}
		else if (exe.pipe_out) {
			//This pipe will be stdin for next process
			kiv_os_rtl::Create_Pipe(last_pipe, exe.out_handle);
		}
		else {
			exe.out_handle = sout;
		}
	}
}

//TODO PROCESS CHECK
std::map<std::string, bool> executables = { {"echo", true}, {"shell", true}, {"freq", true}, {"rgen", true}};

void Execute(std::vector<executable> &exes) {

	for (const executable &exe : exes) {
		size_t handle;
		
		if (executables.find(exe.name) == executables.end()) {
			continue;
		}

		//Pripravime argumenty programu
		std::stringstream args;
		args.str("");
		if (exe.args.empty() == false) {
			args << exe.args[0];
		}
		
		for (int i = 1; i < exe.args.size(); i++) {
			args << ' ' << exe.args[i];
		}

		kiv_os_rtl::Clone(exe.name.c_str(), args.str().c_str(), exe.in_handle, exe.out_handle, handle);
		size_t signaled;
		kiv_os_rtl::Wait_For(&handle, 1, signaled);
		//TODO kontrolovat chyby
	}
}

