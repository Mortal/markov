#include <string>
#include <map>
#include <sstream>
#include <iostream>
#include <array>
#include <queue>
#include <random>

using namespace std;

struct tokenizer {
	typedef istream input_t;
	typedef size_t token_t;

private:
	input_t & src;
	map<string, token_t> tokens;
	map<token_t, string> tokens_rev;
	queue<token_t> buffer;

public:
	const token_t error;
	const token_t bos;
	const token_t eos;

	inline tokenizer(input_t & src)
		: src(src)
		, error(add_token(" error "))
		, bos(add_token(" ^ "))
		, eos(add_token("\n\n"))
	{
	}

	inline token_t translate(string token) {
		return tokens[token];
	}

	inline string translate(token_t token) {
		return tokens_rev[token];
	}

	inline operator bool() {
		return has_input();
	}

	inline bool has_input() {
		if (buffer.size()) return true;
		return get_some_input();
	}

	inline tokenizer & operator>>(token_t & dest) {
		if (!has_input()) return *this;
		dest = pop_input();
	}

	inline token_t pop_input() {
		if (!has_input()) throw "No input";
		token_t val = buffer.front();
		buffer.pop();
		return val;
	}

private:
	inline bool get_some_input() {
		string word;
		if (!(src >> word)) return false;

		push_word(word);
		return true;
	}

	inline token_t add_token(string word) {
		// noop if already exists
		tokens.insert(make_pair(word, tokens.size()));
		tokens_rev[tokens[word]] = word;
		return tokens[word];
	}

	inline void push_word(string word) {
		buffer.push(add_token(word));
	}
};

template <size_t K>
struct kgrams {
	typedef tokenizer tokenizer_t;
	typedef typename tokenizer_t::token_t token_t;
	typedef array<token_t, K> ktoken_t;
	typedef vector<token_t> adjacent_t;
	typedef map<ktoken_t, adjacent_t> edgelist_t;

	inline kgrams(tokenizer_t & tokens)
		: tokens(tokens)
		, current(bos())
		, edgecount(0)
	{
		token_t next;
		while (tokens >> next) {
			// insert edge if not exists, and increment count
			edgelist[current].push_back(next);
			++edgecount;

			// advance
			advance_current_with(next);
		}
		edgelist[current].push_back(tokens.eos);
		++edgecount;

		current = bos();
	}

	inline token_t get_next() {
		if (!edgelist.count(current)) {
			current = bos();
			return tokens.error;
		}
		adjacent_t & adjacents = edgelist[current];
		size_t choice = (adjacents.size() == 1) ? 0 : (r() % adjacents.size());
		token_t tok = adjacents[choice];
		advance_current_with(tok);
		if (tok == tokens.eos) {
			current = bos();
		}
		return tok;
	}

	inline void dump() {
		for (auto i = edgelist.begin(); i != edgelist.end(); ++i) {
			cout << '[' << tokens.translate(i->first[0]);
			for (int j = 1; j < K; ++j) cout << ", " << tokens.translate(i->first[j]);
			cout << "] = {" << tokens.translate(i->second[0]);
			for (int j = 1; j < i->second.size(); ++j) cout << ", " << tokens.translate(i->second[j]);
			cout << "}" << endl;
		}
	}

private:
	tokenizer_t & tokens;
	ktoken_t current;
	edgelist_t edgelist;
	size_t edgecount;
	mt19937 r;

	inline ktoken_t bos() {
		ktoken_t res;
		fill(res.begin(), res.end(), tokens.bos);
		return res;
	}

	inline void advance_current_with(token_t next) {
		copy(current.begin()+1, current.end(), current.begin());
		current[K-1] = next;
	}
};

int main() {
	tokenizer tok(cin);
	kgrams<2> k(tok);
	k.dump();
	while (true) {
		string word = tok.translate(k.get_next());
		cout << word << ' ' << flush;
	}
	return 0;
}
// vim:set ts=4 sts=4 sw=4:
