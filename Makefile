CXXFLAGS=-O3 -std=gnu++0x -Wall -Wextra
all: markov irssicomplete

markov: markov.o markovmain.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

irssicomplete: markov.o irssicomplete.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

irssicomplete_test: markov.o irssicomplete_test.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

test: irssicomplete_test
	./irssicomplete_test
