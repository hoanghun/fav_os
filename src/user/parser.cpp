#include "parser.h"

#include <iostream>
#include <sstream>
#include <iterator>

std::vector<std::string> Parse(const char *line) {
	
	std::istringstream iss(line);

	std::vector<std::string> results(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());
	return results;

}