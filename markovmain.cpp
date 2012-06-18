#include <iostream>
#include "markov.h"

using namespace std;

int main(int argc, char ** argv) {
	if (argc < 2) {
		cout << "Usage: " << argv[0] << " K" << endl;
		return 1;
	}
	string arg(argv[1]);
	return markov(cin, cout, arg, 1000000) ? 0 : 1;
}
// vim:set ts=4 sts=4 sw=4:
