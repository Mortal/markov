#include "markov.h"

struct tests {
	size_t fails;
	size_t n;
	tests() : fails(0), n(0) {}

	void expect(std::string got, std::string target) {
		++n;
		if (got != target) {
			std::cerr << "Test " << n << " failed: got \"" << got << "\", expected \"" << target << '"' << std::endl;
			++fails;
		}
	}

	int go() {
		wordcompleter wc;
		wc.learn("a b");
		wc.learn("c d");
		expect(wc.complete("a", ""), "b");
		expect(wc.complete("c", ""), "d");
		wc.learn("a b");
		wc.learn("a e");
		wc.learn("c f");
		wc.learn("c f");
		expect(wc.complete("a", ""), "b e");
		expect(wc.complete("c", ""), "f d");
		std::cerr << fails << " test" << (fails == 1 ? "" : "s") << " failed" << std::endl;
		return fails == 0 ? 0 : 1;
	}
};

int main() {
	tests t;
	return t.go();
}
// vim:set ts=4 sts=4 sw=4:
