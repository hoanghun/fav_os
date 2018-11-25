#pragma once

#include <vector>
#include <string>

#include "rtl.h"

struct TExecutable {

	std::string name;
	std::vector<std::string> args;

	std::string file_in = "";
	std::string file_out = "";

	bool file_out_rewrite = false;

	bool pipe_in = false;
	bool pipe_out = false;

	kiv_os::THandle in_handle;
	kiv_os::THandle out_handle;

	bool Check() const;
	void Close_Stdin() const;
	void Close_Stdout() const;

};

bool Update_Executable(std::stringstream &strs, TExecutable &item, size_t property);
std::vector<TExecutable> Parse(const char *line, const size_t line_length);