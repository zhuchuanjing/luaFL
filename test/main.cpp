#include "../luaFL.h"
class Http : public LuaWarp<Http> {
public:
	static const char* getLuaName() { return "Http"; }
	static void addClassMember(lua_State* L) {}
	void addObjectMember(lua_State* L) {
		addMember(L, "Write", [](lua_State* L) {
			//getObj(L, 1)->writeHttp(lua_tostring(L, -1));
			return 0;
		});
	}
};

int main(int argc, char** argv) {
	LuaEnv lua;
	Http::registerClass(lua.get());
	lua.load("test.lua");
	Http http;
	auto l = lua.create();
	if(Http::getClassMember(l, "lua") != LUA_TNIL) {
		std::unordered_map<const char*, const char*> query;
		query["xxx"] = "ajshdj";
		query["aaa"] = "aaaaaajshdj";
		query["bbb"] = "xxxxxajshdj";
		http.bind(l);
		http.addMember(l, "params", std::list<int>{10, 20, 40, 80});
		http.addMember(l, "query", query);
		//		addMember(l, "query", values);
		//addMember(l, "query", parser.Params.c_str(query, 256));
		lua_pcall(l, 1, 0, 0);
	}

	return 0;
}