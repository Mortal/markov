#include <string>
#include <map>
#include <sstream>
#include <iostream>
#include <array>
#include <queue>
#include <random>

using namespace std;

struct tokenizer {
	static constexpr const char * ERROR = " error ";
	static constexpr const char * BOS = " ^ ";
	static constexpr const char * EOS = " $ ";
	typedef istream input_t;
	typedef size_t token_t;

	inline tokenizer(input_t & src)
		: src(src)
	{
		tokens[ERROR] = 0;
		tokens[BOS] = 1;
		tokens[EOS] = 2;
		tokens_rev[0] = ERROR;
		tokens_rev[1] = BOS;
		tokens_rev[2] = EOS;
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
	input_t & src;
	map<string, token_t> tokens;
	map<token_t, string> tokens_rev;
	queue<token_t> buffer;

	inline bool get_some_input() {
		string word;
		if (!(src >> word)) return false;

		push_word(word);
		return true;
	}

	inline void push_word(string word) {
		// noop if already exists
		tokens.insert(make_pair(word, tokens.size()));
		tokens_rev[tokens[word]] = word;

		buffer.push(tokens[word]);
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
		token_t eos = tokens.translate(tokenizer_t::EOS);
		edgelist[current].push_back(eos);
		++edgecount;

		current = bos();
	}

	inline string get_next() {
		if (!edgelist.count(current)) {
			current = bos();
			return tokenizer_t::ERROR;
		}
		adjacent_t & adjacents = edgelist[current];
		size_t choice = (adjacents.size() == 1) ? 0 : (r() % adjacents.size());
		token_t tok = adjacents[choice];
		advance_current_with(tok);
		string translated = tokens.translate(adjacents[choice]);
		if (translated == tokenizer_t::EOS) {
			current = bos();
			return "\n\n";
		}
		return translated;
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
		fill(res.begin(), res.end(), tokens.translate(tokenizer_t::BOS));
		return res;
	}

	inline void advance_current_with(token_t next) {
		for (size_t k = 0; k < K-1; ++k) current[k] = current[k+1];
		current[K-1] = next;
	}
};

int main() {
	tokenizer tok(cin);
	kgrams<2> k(tok);
	k.dump();
	while (true) {
		string word = k.get_next();
		cout << word << ' ' << flush;
	}
	return 0;
}
// vim:set ts=4 sts=4 sw=4:
