#ifndef REDIS_H
#define REDIS_H

#include "uvpeer.h"
#include <sstream>
#include <list>

class RedisClient : public UVClient {
    protected:
        struct Bulk {
            char ch;
            int len;
            int number;
            std::string str;
        };

        int getNumber(char* begin, char* end) {
            int number;
            std::stringstream ss;
            ss << std::string(begin, end - begin);
            ss >> number;
            return number;
        }

        bool getBulk(char* buf, int len, Bulk& bulk) {
            char* ptr = buf + 1;
            while(*ptr != '\r' && ptr - buf < len) ptr++;
            if(*ptr != '\r') return false;
            bulk.ch = *buf;
            bulk.number = 0;
            bulk.len = (int)(ptr - buf) + 2;
            if(*buf == '$') {
                int _len = getNumber(buf + 1, ptr);
                if(_len <= 0) bulk.ch = '-';			//-1 means error
                else {
                    bulk.len += _len + 2;
                    if(bulk.len > len) return false;
                    bulk.str = std::string(ptr + 2, _len);
                }
            } else if(*buf == '*') {
                bulk.number = getNumber(buf + 1, ptr);
            } else bulk.str = std::string(ptr + 2, ptr - buf);
            return true;
        }

    public:
        char status;
        std::list<std::string> result;

        std::pair<int, std::string> assemble(const char* value) {
            std::stringstream ss;
            ss << '$' << strlen(value) << "\r\n" << value << "\r\n";
            return std::pair<int, std::string>{1, ss.str()};
        }

        template<typename T> std::pair<int, std::string> assemble(T value) {
            std::stringstream ss;
            ss << value;;
            return assemble(ss.str().c_str());
        }

        template<typename T, typename... Args> std::pair<int, std::string> assemble(T value, Args... args) {
            auto first = assemble(value);
            auto second = assemble(args...);
            return std::pair<int, std::string>{first.first + second.first, first.second + second.second};
        }

		bool call(const std::pair<int, std::string> command) {
			std::stringstream ss;
			ss << '*' << command.first << "\r\n" << command.second;
			UVClient::write(ss.str().c_str(), (int)ss.str().length());
			bool completed = false;
			clear();
			while(!completed && connected) {
				completed = true;
				UVClient::read();
				result.clear();
				Bulk bulk;
				if(connected && data != nullptr){
					if(getBulk(data, data_size, bulk)) {
						status = bulk.ch;
						if(bulk.ch == '*') {
							char* ptr = data  + bulk.len;
							int size = bulk.len;
							for(int index = 0; index < bulk.number; index++) {
								Bulk b;
								if(getBulk(ptr, data_size - size, b)) {
									result.push_back(b.str);
									ptr += b.len;
									size += b.len;
								} else {
									completed = false;
									break;
								}
							}
						} else if(!bulk.str.empty()) result.push_back(bulk.str);
					} else completed = false;
				}
			}
			return connected && status != '-';
		}

        template<typename... Args> bool call(Args... args) {
            auto request = assemble(args...);
			return call(request);
        }
};

#endif
