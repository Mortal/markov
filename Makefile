CXXFLAGS=-O3 -std=gnu++0x -Wall -Wextra
all: markov

markov: markov.o markovmain.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^
