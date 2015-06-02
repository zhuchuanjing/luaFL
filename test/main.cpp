#include "../luaFL.h"
#include "../uvpeer.h"
#include "../util.h"
#include <time.h>

LuaEnv gLua;

const char* QueryKeys[] ={"_callback", "gkey", "qid", "huid", "adkey", nullptr};

class HttpPeer : public UVPeer, public LuaWarp<HttpPeer> {
public:
	bool keepAlive;

	void writeHttp(const char* data = nullptr) {
		time_t t;
		t = time(&t);
		std::stringstream ss;
		char time_buf[256];
		strftime(time_buf, 256, "%Y-%m-%d %H:%M:%S", localtime(&t));
		ss << "HTTP/1.1 200 OK\r\nServer: ADServer\r\nDate:" << time_buf << "Content-Type: text/html; charset=utf-8\r\n";
		if(keepAlive) ss << "Connection: keep-alive\r\n";
		if(data != nullptr) ss << "Content-Length:" << strlen(data) << "\r\n\r\n" << data;
		else ss << "Content-Length:0\r\n\r\n";
		write(ss.str().c_str(), (int)ss.str().length());
	}

	void onConnect() {
		read();
	}

	void onRead() {
		HttpParser parser(data, data_size);
		keepAlive = true;
		for(auto h : parser.Heads) {
			if(h.first.equal("Connection")) {
				if(h.second.equal("close")) keepAlive = false;
				break;
			}
		}
		char uri[256];
		char query_buf[1024];
		auto l = gLua.create();
		if(HttpPeer::getClassMember(l, "get") != LUA_TNIL) {
			std::unordered_map<const char*, const char*> query;
			Parser query_parser(QueryKeys);
			query_parser.process(parser.Params.c_str(query_buf, 1024));
			for(size_t index = 0; index < query_parser.Keys.size(); index++) {
				if(!query_parser.Values[index].empty()) query[query_parser.Keys[index].c_str()] = query_parser.Values[index].c_str();
			}
			bind(l);
			addMember(l, "url", parser.Url.c_str(uri, 256));
			addMember(l, "query", query);
			lua_pcall(l, 1, 0, 0);
		}
	}

	virtual void onClose() {
		delete this;
	}

	virtual void onWrite() {
		if(!keepAlive) close();
		else read();
	}

	static const char* getLuaName() { return "Http"; }
	static void addClassMember(lua_State* L) {}
	void addObjectMember(lua_State* L) {
		addMember(L, "Write", [](lua_State* L) {
			getObj(L, 1)->writeHttp(lua_tostring(L, -1));
			return 0;
		});
	}
};

#include "../redis.h"
class Redis : public RedisClient, public LuaWarp<Redis> {
public:
	static const char* getLuaName() { return "Redis"; }
	static void addClassMember(lua_State* L) {
		addMember(L, "New", [](lua_State* L) {
			const char* host = "127.0.0.1";
			int port = 6379;
			if(lua_gettop(L) >= 1 && lua_type(L, 1) == LUA_TSTRING) host = lua_tostring(L, 1);
			if(lua_gettop(L) >= 2 && lua_type(L, 2) == LUA_TNUMBER) port = (int)lua_tointeger(L, 2);
			Redis* r = new Redis;
			r->connect(host, port);
			r->bind(L);
			return 1;
		});
	}
	void addObjectMember(lua_State* L) {
		addMember(L, "Call", [](lua_State* L) {
			auto r = getObj(L, 1);
			std::pair<int, std::string> command{0, ""};
			for(int index = 2; index <= lua_gettop(L); index++) {
				if(lua_type(L, index) == LUA_TSTRING) {
					auto ret = r->assemble(lua_tostring(L, index));
					command.first += ret.first;
					command.second += ret.second;
				}
				if(lua_type(L, index) == LUA_TNUMBER) {
					auto ret = r->assemble(lua_tointeger(L, index));
					command.first += ret.first;
					command.second += ret.second;
				}
			}
			if(r->call(command)) {
				if(r->result.size() == 1) r->add(L, r->result.front());
				else if(r->result.size() > 1) {
					lua_newtable(L);
					for(auto item : r->result) {
						r->add(L, item);
					}
				} else lua_pushnil(L);
			} else lua_pushnil(L);
			return 1;
		});
		addMember(L, "Delete", [](lua_State* L) {
			delete getObj(L, 1);
			return 0;
		});
	}
};

int main(int argc, char** argv) {
	HttpPeer::registerClass(gLua.get());
	Redis::registerClass(gLua.get());
	//lua.load("/Users/zhuchuanjing/Desktop/future/luaFL/test/test.lua");
	gLua.load("test.lua");
	RedisClient r;
	if(r.connect("127.0.0.1", 6379) && r.call("get", "aaa")) {
		for(auto item : r.result) {
			std::cout << item << std::endl;
		}
	}
	UVServer<HttpPeer> server;
	if(server.start("127.0.0.1", 9001)) {
		return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	}
	return 0;
}