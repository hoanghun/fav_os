#include "parser.h"

#include <sstream>
#include <iterator>

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