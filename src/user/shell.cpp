#include "shell.h"
#include "rtl.h"
#include "stdio.h"

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

			const char* new_line = "\n";
			kiv_std_lib::Print_Line(regs, new_line, strlen(new_line));
			kiv_std_lib::Print_Line(regs, buffer, strlen(buffer));
			kiv_std_lib::Print_Line(regs, new_line, strlen(new_line));
		}
		else
			break;	//EOF
	} while (strcmp(buffer, "exit") != 0);

	kiv_os_rtl::Exit(0);

}