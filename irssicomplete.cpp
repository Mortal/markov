#include <sstream>
#include "markov.h"

int main() {
	wordcompleter wc;
	std::string line;
	while (getline(std::cin, line)) {
		std::stringstream ss(line);
		std::string firstword;
		ss >> firstword;
		ss.get(); // discard space
		std::string rest;
		getline(ss, rest);
		if (firstword == "COMPLETE") {
			std::string linestart;
			getline(std::cin, linestart);
			std::string result = wc.complete(linestart, rest);
			std::cout << result << std::endl;
		} else if (firstword == "LEARN") {
			wc.learn(rest);
		} else {
			std::cout << "Unknown command '" << firstword << "'" << std::endl;
		}
	}
	return 0;
}
// vim:set ts=4 sts=4 sw=4:
