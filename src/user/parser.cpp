#include "parser.h"

#include <iostream>
#include <sstream>
#include <iterator>

//std::vector<std::string> split(const std::string &s, char delimiter) {
//	std::vector<std::string> tokens;
//	std::string token;
//	std::istringstream tokenStream(s);
//	while (std::getline(tokenStream, token, delimiter)) {
//		tokens.push_back(token);
//	}
//
//	return tokens;
//}

bool TExecutable::Check() const {

	if (name.length() == 0) {
		return false;
	}

	if (file_in.length() > 0 && pipe_in == true) {
		return false;
	}

	if (file_out.length() > 0 && pipe_out == true) {
		return false;
	}

	return true;
}

void TExecutable::Close_Stdin() const {

	if (file_in.length() > 0 || pipe_in == true) {
		if (in_handle > 0) {
			kiv_os_rtl::Close_Handle(in_handle);
		}
	}

}

void TExecutable::Close_Stdout() const {

	if (file_out.length() > 0 || pipe_out == true) {
		if (out_handle > 0) {
			kiv_os_rtl::Close_Handle(out_handle);
		}
	}

}

bool Update_Executable(std::stringstream  &strs, TExecutable &item, size_t property) {

	//Pokud neni stringstream prazdny
	if (strs.rdbuf()->in_avail() != 0) {

		std::string str = strs.str();

		switch (property) {
		case 0:
			item.name = str;
			break;
		case 1:
			item.args.push_back(str);
			break;
		case 2:
			item.file_in = str;
			break;
		case 3:
			item.file_out = str;
			break;
		}

		strs.str("");
		return true;
	}

	strs.str("");
	return false;

}

std::vector<TExecutable> Parse(const char *line, const size_t line_length) {
	
	std::vector<TExecutable> items;

	size_t property = 0;
	TExecutable item;
	std::stringstream strs;

	for (int i = 0; i < line_length; i++) {

		if (line[i] == '|') {
			
			Update_Executable(strs, item, property);

			item.pipe_out = true;
			items.push_back(item);

			item = TExecutable();
			item.pipe_in = true;

			property = 0;
		}
		else if (line[i] == '<') {
			
			Update_Executable(strs, item, property);
			property = 2;

		}
		else if (line[i] == '>') {
			Update_Executable(strs, item, property);

			if (property == 3 && line[i - 1] == '>') {
				item.file_out_rewrite = true;
			}

			property = 3;
		}
		else if (line[i] == ' ') {
			if (Update_Executable(strs, item, property) == true) {
				property = 1;
			}
		}
		else {
			strs << line[i];
		}
	}

	//Ulozit posledni slovo a item
	Update_Executable(strs, item, property);
	items.push_back(item);

	return items;

}