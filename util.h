#ifndef UTIL_H
#define UTIL_H

#include <list>
#include <functional>
#include <string.h>

#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>

class Parser {
	std::string getString(const char* ptr) {
		const char *start = ptr;
		while(*ptr != '"') {
			if(*ptr == 0) return "";
			ptr++;
		}
		return std::string(start, ptr);
	}
	const char* skipSpace(const char*ptr) {
		while(*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n') ptr++;
		return ptr;
	}
public:
	std::vector<std::string> Keys;
	std::vector<std::string> Values;
	Parser(const char* keys[]) {
		while(*keys != nullptr) {
			Keys.push_back(*keys);
			keys++;
		}
		Values.resize(Keys.size());
	}

	bool empty() {
		for(auto v : Values) if(v.empty()) return true;
		return false;
	}

	Parser(const char* keys[], const char* file_name) : Parser(keys) {
		std::ifstream in(file_name);
		if(in.is_open()) {
			while(!in.eof()) {
				std::string line;
				std::getline(in, line);
				process(line.c_str());
			}
		}
	}

	static int getInt(const std::string str) {
		if(str.empty()) return 0;
		else return atoi(str.c_str());
	}

	int getInt(int index) {
		return getInt(Values[index]);
	}

	std::string getStr(int index) {
		if(!Values[index].empty()) {
               return Values[index];
    	}
        return "";
    }

	std::string getUserID(const char* ip) {
		std::string id = ip;
		for(int index = 0; index < (int)Keys.size(); index++) {
			if(Keys[index] == "qid" || Keys[index] == "huid" || Keys[index] == "mid" || Keys[index] == "__ip") {
				if(!Values[index].empty()) {
					id = Values[index];
					break;
				}
			}
		}
		return id;
	}

	static std::vector<std::string>split(const std::string str, const char ch = '|') {
		std::vector<std::string> result;
		int pos = 0;
		while(pos != std::string::npos) {
			auto end = str.find_first_of(ch, pos);
			if(end != std::string::npos) {
				result.push_back(str.substr(pos, end - pos));
				pos = end + 1;
			} else {
				result.push_back(str.substr(pos, str.length() - pos));
				break;
			}
		}
		return result;
	}

	std::vector<std::string>split(int index, const char ch = '|') {
		return split(Values[index], ch);
	}

	int process(const char* params) {
		if(!params) return -1;
		int number = 0;
		while(*params != 0) {
			unsigned int index = 0;
			for(index = 0; index < Keys.size(); index++) {
				if(!memcmp(params, Keys[index].c_str(), Keys[index].size()) && (*(params + Keys[index].size()) == '=' || *(params + Keys[index].size()) == ' ')) {
					params += Keys[index].size();
					while(*params == ' ') params++;
					if(*params != '=') continue;
					params++;
					while(*params == ' ') params++;
					auto ptr = params;
					while(*ptr != '&' && *ptr != ' ' && *ptr != '\r' && *ptr != '\n' && *ptr != 0) ptr++;
					Values[index] = std::string(params, ptr - params);
					number++;
					params = ptr;
					if(*params != 0) params++;
					while(*params == ' ' || *params == '\r' || *params == '\n') params++;
					break;
				}
			}
			if(index == Keys.size()) {
				while(*params != 0 && *params != '&') params++;
				if(*params == '&') params++;
			}
		}
		return number;
	}

	bool add(const std::string key, const std::string value) {
		if(!value.empty()) {
			for(int index = 0; index < (int)Keys.size(); index++) {
				if(Keys[index].compare(key) == 0) {
					Values[index] = value;
					return true;;
				}
			}
		}
		return false;
	}

	int processJson(const std::string params) {
		return processJson(params.c_str(), params.length());
	}
	int processJson(const char* params, int length) {
		int number = 0;
		const char* p = params;
		while(p - params < length) {
			if(*p == '"') {
				auto key = getString(p + 1);
				if(key.empty()) return number;
				p = skipSpace(p + key.length() + 2);
				if(*p != ':') break;
				else p++;
				p = skipSpace(p);
				if(*p == '"') {
					auto value = getString(p + 1);
					if(add(key, value)) number++;
					p += value.length() + 2;
				} else if(*p >= '0' && *p <= '9') {
					auto start = p;
					while((*p >= '0' && *p <= '9') || *p == 'x' || *p == 'X') p++;
					auto value = std::string(start, p);
					if(add(key, value)) number++;
				} else if(*p == '{') {
					p++;
				}
			} else p++;
		}
		return number;
	}
};

#include <list>
struct str_blk {
	const char* ptr;
	int len;
	void step() { ptr++; len--; }
	bool equal(const char* str) {
		int size = 0;
		while(*(str + size) != 0 && size < len) {
			char a = *(str + size);
			if(a >= 'a' && a <= 'z') a += 'A' - 'a';
			char b = *(ptr + size);
			if(b >= 'a' && b <= 'z') b += 'A' - 'a';
			if(a != b) return false;
			size++;
		}
		return size == len;
	}

	int getHex(char ch) {
		if(ch >= '0' && ch <= '9') return ch - '0';
		else if(ch >= 'a' && ch <= 'f') return (ch - 'a') + 1;
		else if(ch >= 'A' && ch <= 'F') return (ch - 'a') + 1;
		return -1;
	}

	char* c_str(char* buf, int size) {
		int index = 0;
		int dest = 0;
		while(index < len && dest < size - 1) {
			if(ptr[index] == '%') {
				int value = -1;
				if(++index < len) {
					value = getHex(ptr[index]);
					if(value >= 0 && ++index < len) {
						int x = getHex(ptr[index]);
						if(x >= 0) value = value * 0x10 + x;
					}
				}
				if(value < 0) break;
				else {
					buf[dest++] = value;
					index++;
				}
			} else buf[dest++] = ptr[index++];
		}
		buf[dest] = 0;
		return buf;
	}
};

class HttpParser {
	void skipSpace(str_blk& blk) {
		while(blk.len > 0 && (*blk.ptr == ' ' || *blk.ptr == '\t')) blk.step();
	}

	str_blk getLine(str_blk& blk) {
		skipSpace(blk);
		const char* start = blk.ptr;
		while(blk.len > 0 && *blk.ptr != '\r' && *blk.ptr != '\n') blk.step();
		str_blk ret{start, (int)(blk.ptr - start)};
		if(blk.len > 0 && *blk.ptr == '\r') blk.step();
		if(blk.len > 0 && *blk.ptr == '\n') blk.step();
		return ret;
	}

	str_blk split(str_blk& blk, char ch = ' ') {
		skipSpace(blk);
		const char* start = blk.ptr;
		while(blk.len > 0 && *blk.ptr != ch) blk.step();
		str_blk ret{start, (int)(blk.ptr - start)};
		if(blk.len > 0 && *blk.ptr == ch) blk.step();
		return ret;
	}

public:
	str_blk Cmd;
	str_blk Url;
	str_blk Params;
	str_blk Ver;
	str_blk Content;
	std::list<std::pair<str_blk, str_blk>> Heads;
	HttpParser(const char* buf, int len) {
		Content.ptr = buf;
		Content.len = len;
		str_blk line = getLine(Content);
		Cmd = split(line);
		str_blk _url = split(line);
		Url = split(_url, '?');
		Params = _url;
		do {
			line = getLine(Content);
			if(line.len > 0) {
				str_blk key = split(line, ':');
				skipSpace(line);
				Heads.push_back(std::pair<str_blk, str_blk>{key, line});
			} else {
				for(auto h : Heads) {
					if(h.first.equal("Transfer-Encoding") && h.second.equal("chunked")) {
						line = getLine(Content);
						char buf[64];
						int len = strtol(line.c_str(buf, 64), NULL, 16);
						while(*Content.ptr == '\r' || *Content.ptr == '\n') Content.step();
						Content.len = len;
						break;
					}
				}
				break;
			}
		} while(Content.len > 0);
	}
};

#endif
