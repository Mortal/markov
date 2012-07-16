#include <iostream>
#include <string>
#include <random>

struct wordcompleter {
	static wordcompleter * create();
	static void destroy(wordcompleter *);

	void learn(const std::string & line);
	std::string complete(const std::string & linestart, const std::string & word);
protected:
	wordcompleter() {};
	~wordcompleter() {};
};

bool markov(std::istream & is, std::ostream & os, std::string arg, size_t lines, std::mt19937 & rng);
bool markov(std::istream & is, std::ostream & os, std::string arg, size_t lines);
// vim:set ts=4 sts=4 sw=4:
