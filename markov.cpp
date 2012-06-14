#include <string>
#include <map>
#include <sstream>
#include <iostream>
#include <array>
#include <queue>
#include <random>
#include <cctype> // isalnum

using namespace std;

struct tokenizer {
	typedef istream input_t;
	typedef size_t token_t;

private:
	input_t & src;
	size_t specials; // number of special tokens
	map<string, token_t> tokens;
	map<token_t, string> tokens_rev;
	queue<token_t> buffer;

public:
	const token_t error = add_special_token();
	const token_t bos = add_special_token();
	const token_t eos = add_special_token();
	const token_t quote = add_special_token();
	const token_t unquote = add_special_token();
	const token_t hyphen = add_special_token();

	inline tokenizer(input_t & src)
		: src(src)
		, specials(0)
	{
	}

	inline string translate(token_t token) {
		if (token == error) return "ERROR";
		if (token == bos) return " ^ ";
		if (token == eos) return " $ ";
		if (token == quote) return "\"";
		if (token == unquote) return "\"";
		if (token == hyphen) return "-";
		return tokens_rev[token];
	}

	inline bool postjoiner(string word) {
		if (word.size() != 1) return false;
		switch (word[0]) {
			case ',':
			case '!':
			case '?':
			case '.':
			case ':':
			case ';':
				return true;
		}
		return false;
	}

	inline bool prejoiner(string word) {
		return word == "$";
	}

	inline string translate_with(token_t token, token_t prev) {
		if (token == eos) return "";
		string word = translate(token);
		if (prev == eos) return "\n"+word;
		if (prev == bos
			|| prev == quote
			|| token == unquote
			|| postjoiner(word)
			|| token == hyphen
			|| prev == hyphen) return word;
		if (prejoiner(translate(prev))) return word;
		return ' '+word;
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
		return *this;
	}

	inline token_t pop_input() {
		if (!has_input()) throw "No input";
		token_t val = buffer.front();
		buffer.pop();
		return val;
	}

private:
	inline bool isalnum(char c) {
		if (std::isalnum(c) || c == '\'') return true;
		return false;
	}

	inline bool get_some_input() {
		string line;
		if (!getline(src, line)) return false;
		enum state {
			NOWORD,
			ENDWORD,
			WORD,
			URL
		};
		state s = NOWORD;
		stringstream wordbuf;
		for (size_t i = 0; i < line.size(); ++i) {
			if (!isprint(line[i])) continue;
			switch (s) {
				case ENDWORD:
					s = NOWORD;
					if (line[i] == '"') {
						push_token(unquote);
						break;
					}
					if (line[i] == '-') {
						push_token(hyphen);
						break;
					}
					if (postjoiner(string(1, line[i]))) {
						push_word(string(1, line[i]));
						s = ENDWORD;
						break;
					}
					// FALLTHROUGH
				case NOWORD:
					if (line[i] == '"') {
						push_token(quote);
					} else if (isalnum(line[i]) || line[i] == ':') {
						s = WORD;
						wordbuf.str("");
						wordbuf << line[i];
					} else if (!isspace(line[i])) {
						push_word(string(1, line[i]));
					}
					break;
				case WORD:
					if (!isalnum(line[i])) {
						string word = wordbuf.str();
						if ((word == "http" || word == "https") && line[i] == ':') {
							s = URL;
						} else {
							push_word(word);
							s = ENDWORD;
							--i;
						}
					} else {
						wordbuf << line[i];
					}
					break;
				case URL:
					if (line[i] == ' ') {
						s = NOWORD;
					}
					break;
			}
		}
		push_token(eos);

		return true;
	}

	inline token_t add_token(string word) {
		// noop if already exists
		tokens.insert(make_pair(word, tokens.size()+specials));
		tokens_rev[tokens[word]] = word;
		return tokens[word];
	}

	inline token_t add_special_token() {
		return tokens.size()+(specials++);
	}

	inline void push_token(token_t token) {
		buffer.push(token);
	}

	inline void push_word(string word) {
		push_token(add_token(word));
	}
};

struct chartokenizer {
	typedef istream input_t;
	typedef size_t token_t;

	const token_t error = 0;
	const token_t bos = 1;
	const token_t eos = 10;

	inline chartokenizer(input_t & src)
		: src(src)
	{
	}

	inline string translate(token_t token) {
		if (token == error) return "ERROR";
		if (token == bos) return " ^ ";
		if (token == eos) return "\n";
		return string(1, static_cast<char>(token));
	}

	inline string translate_with(token_t token, token_t prev) {
		if (token == eos && prev == eos) return "";
		return translate(token);
	}

	inline operator bool() {
		return has_input();
	}

	inline bool has_input() {
		if (buffer.size()) return true;
		char c;
		src.get(c);
		if (!src.good()) return false;
		push_token(c);
		return true;
	}

	inline chartokenizer & operator>>(token_t & dest) {
		if (!has_input()) return *this;
		dest = pop_input();
		return *this;
	}

	inline token_t pop_input() {
		if (!has_input()) throw "No input";
		token_t val = buffer.front();
		buffer.pop();
		return val;
	}

private:
	input_t & src;
	queue<token_t> buffer;
	inline void push_token(token_t token) {
		buffer.push(token);
	}

};

template <size_t K, typename tokenizer_t>
struct kgrams {
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
			if (next == tokens.eos) current = bos();
			else advance_current_with(next);
		}
		if (current != bos()) edgelist[current].push_back(tokens.eos);
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
			for (size_t j = 1; j < K; ++j) cout << ", " << tokens.translate(i->first[j]);
			cout << "] = {" << tokens.translate(i->second[0]);
			for (size_t j = 1; j < i->second.size(); ++j) cout << ", " << tokens.translate(i->second[j]);
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

template <size_t K, typename tokenizer_t>
void go() {
	tokenizer_t tok(cin);
	kgrams<K, tokenizer_t> k(tok);
	//k.dump();
	auto prev = tok.bos;
	while (true) {
		auto cur = k.get_next();
		cout << tok.translate_with(cur, prev);
		prev = cur;
	}
}

int main(int argc, char ** argv) {
	if (argc < 2) {
		cout << "Usage: " << argv[0] << " K" << endl;
		return 1;
	}
	string arg(argv[1]);
	if (arg == "1") go<1, tokenizer>();
	else if (arg == "2") go<2, tokenizer>();
	else if (arg == "3") go<3, tokenizer>();
	else if (arg == "4") go<4, tokenizer>();
	else if (arg == "5") go<5, tokenizer>();
	else if (arg == "c1") go<1, chartokenizer>();
	else if (arg == "c2") go<2, chartokenizer>();
	else if (arg == "c3") go<3, chartokenizer>();
	else if (arg == "c4") go<4, chartokenizer>();
	else if (arg == "c5") go<5, chartokenizer>();
	else if (arg == "c6") go<6, chartokenizer>();
	else if (arg == "c7") go<7, chartokenizer>();
	else if (arg == "c8") go<8, chartokenizer>();
	else if (arg == "c9") go<9, chartokenizer>();
	else if (arg == "parse") {
		tokenizer tok(cin);
		auto prev = tok.bos;
		auto cur = tok.bos;
		while (tok >> cur) {
			cout << tok.translate_with(cur, prev);
			prev = cur;
		}
	}
	else return 1;
	return 0;
}
// vim:set ts=4 sts=4 sw=4:
