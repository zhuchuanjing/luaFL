#include "lua_wrap.h"

#include <list>
#include <thread>

#include <iostream>
lua_State* _gL;

std::list<std::pair<int,int>> _timers;
void tickLua() {
    while(true) {
        int index = 1;
        for(auto& t : _timers) {
            if(t.second == t.first) {
                t.second = 0;
				if(_gL != nullptr && lua_getglobal(_gL, "__timers") != LUA_TNIL) {
					lua_pushinteger(_gL, index);
					lua_rawget(_gL, -2);
					lua_call(_gL, 0, 0);
                }
            } else t.second++;
            index++;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

lua_State* initLua() {
    _gL = luaL_newstate();
	if(_gL != nullptr) {
		luaL_openlibs(_gL);
		lua_pushcfunction(_gL, [](lua_State* L) {
            for(int index = 1; index <= lua_gettop(L); index++) {
                if(lua_isinteger(L, index)) std::cout << lua_tointeger(L, index);
                else if(lua_isstring(L, index)) std::cout << lua_tostring(L, index);
            }
            std::cout << std::endl;
            return 0;
        });
		lua_setglobal(_gL, "Print");
		lua_pushcfunction(_gL, ([](lua_State* L) {
            _timers.push_back(std::pair<int, int>{(int)lua_tointeger(L, 1), 0});
            if(lua_getglobal(L, "__timers") == LUA_TNIL) {
                lua_newtable(L);
                lua_pushvalue(L, -1);
                lua_setglobal(L, "__timers");
            }
            lua_pushinteger(L, _timers.size());
            lua_pushvalue(L, 2);
            lua_rawset(L, -3);
            return 0;
        }));
		lua_setglobal(_gL, "AddTimer");
		return _gL;
    }
    return nullptr;
}

lua_State* getLua() {
	return lua_newthread(_gL);
}

void runLua(const char* name) {
	if(_gL != nullptr) {
		lua_getglobal(_gL, "__G__TRACKBACK__");
		int errfunc = lua_gettop(_gL);
		luaL_loadfile(_gL, name);
		lua_pcall(_gL, 0, 0, errfunc);
	}
}