#include <string>
#include <map>
#include <sstream>
#include <iostream>
#include <array>
#include <queue>
#include <random>
#include <cctype> // isalnum
#include "markov.h"

using namespace std;

template <typename child_t, typename token_t>
struct tokenizer_base {
	typedef token_t token_type;

private:
	queue<token_t> buffer;
	bool went_well;

	inline child_t & self() { return *reinterpret_cast<child_t *>(this); }

protected:
	inline void push_token(token_t token) {
		buffer.push(token);
	}

public:
	inline operator bool() {
		return went_well;
	}

	inline bool has_input() {
		if (buffer.size()) return true;
		self().get_some_input();
		return 0 < buffer.size();
	}

	inline child_t & operator>>(token_t & dest) {
		went_well = has_input();
		if (!went_well) return self();
		dest = buffer.front();
		buffer.pop();
		return self();
	}

	inline void dump() {
		cout << "Tokenizer [" << buffer.size() << "] = {";
		size_t n = buffer.size();
		deque<token_t> items;
		for (size_t i = 0; i < n; ++i) {
			if (i > 0) cout << ", ";
			cout << self().translate(buffer.front());
			items.push_back(buffer.front());
			buffer.pop();
		}
		cout << '}' << endl;
		buffer = queue<token_t>(items);
	}
};

struct tokenizer : public tokenizer_base<tokenizer, size_t> {
	typedef istream input_t;
	typedef token_type token_t;

private:
	input_t & src;
	size_t specials; // number of special tokens
	map<string, token_t> tokens;
	map<token_t, string> tokens_rev;

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

private:
	friend struct tokenizer_base<tokenizer, token_t>;

	inline bool isalnum(char c) {
		if (std::isalnum(c) || c == '\'') return true;
		return false;
	}

	inline void get_some_input() {
		string line;
		if (!getline(src, line)) return;
		use_line(line);
		push_token(eos);
	}

public:
	void use_line(const string & line) {
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
	}

private:
	inline token_t add_token(string word) {
		// noop if already exists
		tokens.insert(make_pair(word, tokens.size()+specials));
		tokens_rev[tokens[word]] = word;
		return tokens[word];
	}

	inline token_t add_special_token() {
		return tokens.size()+(specials++);
	}

	inline void push_word(string word) {
		push_token(add_token(word));
	}
};

struct chartokenizer : public tokenizer_base<chartokenizer, char> {
	typedef istream input_t;
	typedef token_type token_t;

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

private:
	friend struct tokenizer_base<chartokenizer, token_t>;
	input_t & src;

	inline void get_some_input() {
		char c;
		src.get(c);
		if (!src.good()) return;
		push_token(c);
	}

};

template <size_t K, typename tokenizer_t>
struct kgrams {
	typedef typename tokenizer_t::token_t token_t;
	typedef array<token_t, K> ktoken_t;
	typedef vector<token_t> adjacent_t;
	typedef map<ktoken_t, adjacent_t> edgelist_t;

	inline kgrams(tokenizer_t & tokens, mt19937 & rng)
		: tokens(tokens)
		, edgecount(0)
		, r(rng)
	{
		slurp_tokens(tokens);
	}

	void slurp_tokens(tokenizer_t & tokens) {
		token_t next;
		current = bos();
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

	inline adjacent_t get_next_pool() {
		if (!edgelist.count(current)) {
			return adjacent_t();
		}
		adjacent_t & adjacents = edgelist[current];
		return adjacents;
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

	inline void set_state(string with) {
		tokens.use_line(with+' ');
		current = bos();
		token_t next;
		while (tokens >> next) {
			advance_current_with(next);
		}
	}

private:
	tokenizer_t & tokens;
	ktoken_t current;
	edgelist_t edgelist;
	size_t edgecount;
	mt19937 & r;

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

struct executor {
	executor(istream & is, ostream & os, size_t lines, mt19937 & rng)
		: is(is)
		, os(os)
		, lines(lines)
		, rng(rng)
	{
	}

	istream & is;
	ostream & os;
	size_t lines;
	mt19937 & rng;

template <size_t K, typename tokenizer_t>
void go() {
	tokenizer_t tok(is);
	kgrams<K, tokenizer_t> k(tok, rng);
	//k.dump();
	auto prev = tok.bos;
	while (true) {
		auto cur = k.get_next();
		string out = tok.translate_with(cur, prev);
		if (out.find_first_of('\n') != string::npos)
			--lines;
		os << out;
		if (!lines)
			break;
		prev = cur;
	}
}
};

bool markov(istream & is, ostream & os, string arg, size_t lines, mt19937 & rng) {
	executor e(is, os, lines, rng);
	if (arg == "1") e.go<1, tokenizer>();
	else if (arg == "2") e.go<2, tokenizer>();
	else if (arg == "3") e.go<3, tokenizer>();
	else if (arg == "4") e.go<4, tokenizer>();
	else if (arg == "5") e.go<5, tokenizer>();
	else if (arg == "c1") e.go<1, chartokenizer>();
	else if (arg == "c2") e.go<2, chartokenizer>();
	else if (arg == "c3") e.go<3, chartokenizer>();
	else if (arg == "c4") e.go<4, chartokenizer>();
	else if (arg == "c5") e.go<5, chartokenizer>();
	else if (arg == "c6") e.go<6, chartokenizer>();
	else if (arg == "c7") e.go<7, chartokenizer>();
	else if (arg == "c8") e.go<8, chartokenizer>();
	else if (arg == "c9") e.go<9, chartokenizer>();
	else if (arg == "parse") {
		tokenizer tok(is);
		auto prev = tok.bos;
		auto cur = tok.bos;
		while (tok >> cur) {
			os << tok.translate_with(cur, prev);
			prev = cur;
		}
	}
	else return false;
	return true;
}

bool markov(istream & is, ostream & os, string arg, size_t lines) {
	mt19937 rng;
	return markov(is, os, arg, lines, rng);
}

struct wordcompleter::impl {
	static const size_t K = 1;
	typedef kgrams<K, tokenizer> k_t;

	impl()
		: tokens(dummysource)
		, k(tokens, rng)
	{
	}

	void learn_i(const std::string & line) {
		tokens.use_line(line+' ');
		k.slurp_tokens(tokens);
	}

	std::string complete_i(const std::string & linestart, const std::string & word) {
		k.set_state(linestart);
		k_t::adjacent_t adjacents = k.get_next_pool();
		map<k_t::token_t, size_t> counts;
		for (auto i = adjacents.begin(); i != adjacents.end(); ++i) {
			counts.insert(make_pair(*i, 0)).first->second++;
		}
		priority_queue<pair<size_t, k_t::token_t> > tops;
		for (auto i = counts.begin(); i != counts.end(); ++i) {
			tops.push(make_pair(i->second, i->first));
		}
		stringstream result;
		for (size_t i = 0; i < 3; ++i) {
			if (!tops.size()) break;
			if (i) result << ' ';
			result << tokens.translate(tops.top().second);
			tops.pop();
		}
		return result.str();
	}

	mt19937 rng;
	std::stringstream dummysource;
	tokenizer tokens;
	kgrams<K, tokenizer> k;
};

wordcompleter::wordcompleter() {
	pimpl = new wordcompleter::impl();
}

wordcompleter::~wordcompleter() {
	delete pimpl;
}

void wordcompleter::learn(const std::string & line) {
	pimpl->learn_i(line);
}

std::string wordcompleter::complete(const std::string & linestart, const std::string & word) {
	return pimpl->complete_i(linestart, word);
}

// vim:set ts=4 sts=4 sw=4:
