#include "parser.h"

#include <iostream>
#include <sstream>
#include <iterator>

struct executable {

	std::string name;
	std::vector<std::string> args;

	std::string file_in = "";
	std::string file_out = "";
	
	int pipe_in = -1;
	int pipe_out = -1;

};

std::vector<std::string> split(const std::string &s, char delimiter) {
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
}

std::vector<std::string> Parse(const char *line) {
	
	std::vector<std::string> tokens = split(line, '|');
	
	//TODO
	for (auto token : tokens) {

	}

	//TODO
	return tokens;
}