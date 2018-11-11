#include "shell.h"
#include "rtl.h"
#include "stdio.h"
#include "parser.h"

#include <sstream>

size_t __stdcall shell(const kiv_hal::TRegisters &regs) {
	const size_t buffer_size = 256;
	char buffer[buffer_size];
	size_t counter;
	
	const char* intro = "Vitejte v kostre semestralni prace z KIV/OS.\n" \
						"Shell zobrazuje echo zadaneho retezce. Prikaz exit ukonci shell.\n";
	
	kiv_std_lib::Print_Line(regs, intro, strlen(intro));
	const char* prompt = "C:\\>";
	do {
		kiv_std_lib::Print_Line(regs, prompt, strlen(prompt));

		counter = kiv_std_lib::Read_Line(regs, buffer, buffer_size);
		if (counter > 0) {
			if (counter == buffer_size) counter--;
			buffer[counter] = 0;	//udelame z precteneho vstup null-terminated retezec

			std::vector<executable> items = Parse(buffer, strlen(buffer));

			const char* new_line = "\n";
			kiv_std_lib::Print_Line(regs, new_line, strlen(new_line));
			kiv_std_lib::Print_Line(regs, buffer, strlen(buffer));
			kiv_std_lib::Print_Line(regs, new_line, strlen(new_line));
		}
		else
			break;	//EOF
	} while (strcmp(buffer, "exit") != 0);

	kiv_os_rtl::Exit(0);


	return 0;
}

//Pripravime soubory a pipy
void Prepare_For_Execution(std::vector<executable> &exes, kiv_os::THandle sin, kiv_os::THandle sout) {

	//TODO zkontrolovat zda jsou exes validni

	kiv_os::THandle last_pipe = 0;
	for (executable &exe : exes) {
	
		if (exe.file_in.empty() == false) {
			//TODO otevrit soubor
		}
		else if (exe.pipe_in) {
			//TODO otevrit pipe a ulozit handler
			exe.in_handle = last_pipe;
		}
		else {
			exe.in_handle = sin;
		}

		if (exe.file_out.empty() == false) {
			if (exe.file_out_rewrite) {

			}
			else {
			//TODO otevrit soubor
			}
		}
		else if (exe.pipe_out) {
			//TODO otevrit pipe a ulozit handler
			//last_pipe = exe.pipe_out;
		}
		else {
			exe.out_handle = sout;
		}
	}
}

void Execute(std::vector<executable> &exes) {

	for (const executable &exe : exes) {
		size_t handle;

		//Pripravime argumenty programu
		std::stringstream args;
		args.str("");
		for (std::string arg: exe.args) {
			args << ' ' << arg;
		}

		kiv_os_rtl::Clone(exe.name.c_str(), args.str().c_str(), exe.in_handle, exe.out_handle, handle);
		//TODO kontrolovat chyby
	}
}

