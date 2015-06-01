#include "../luaFL.h"
class Http : public LuaWarp<Http> {
    void write(const char* str) {
        std::cout << str << std::endl;
    }
public:
	static const char* getLuaName() { return "Http"; }
	static void addClassMember(lua_State* L) {}
	void addObjectMember(lua_State* L) {
		addMember(L, "Write", [](lua_State* L) {
			getObj(L, 1)->write(lua_tostring(L, -1));
			return 0;
		});
	}
};

int main(int argc, char** argv) {
	LuaEnv lua;
	Http::registerClass(lua.get());
	lua.load("/Users/zhuchuanjing/Desktop/future/luaFL/test/test.lua");
	Http http;
	auto l = lua.create();
	if(Http::getClassMember(l, "lua") != LUA_TNIL) {
		std::unordered_map<const char*, const char*> query;
		query["xxx"] = "ajshdj";
		query["aaa"] = "aaaaaajshdj";
		query["bbb"] = "xxxxxajshdj";
		http.bind(l);
        http.addMember(l, "params", std::list<std::string>{"10", "你好吗", "40", "God Save Me"});
		http.addMember(l, "query", query);
		//		addMember(l, "query", values);
		//addMember(l, "query", parser.Params.c_str(query, 256));
		lua_pcall(l, 1, 0, 0);
	}

	return 0;
}