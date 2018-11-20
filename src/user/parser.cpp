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

bool Update_Executable(std::stringstream  &strs, executable &item, size_t property) {

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

std::vector<executable> Parse(const char *line, const size_t line_length) {
	
	std::vector<executable> items;

	size_t property = 0;
	executable item;
	std::stringstream strs;

	for (int i = 0; i < line_length; i++) {

		if (line[i] == '|') {
			
			Update_Executable(strs, item, property);

			item.pipe_out = true;
			items.push_back(item);

			item = executable();
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