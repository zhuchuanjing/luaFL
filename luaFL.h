#ifndef LUAWRAP_H
#define LUAWRAP_H

#include "lua/lua.hpp"
#include <list>
#include <unordered_map>
#include <sstream>

template<typename T> class LuaWarp {
public:
	static T* getObj(lua_State* L, int index = -1) {
		if(index == -1) index = lua_gettop(L);
		lua_pushstring(L, "__object");
		lua_rawget(L, index);
		T* obj = (T*)lua_touserdata(L, -1);
		lua_pop(L, 1);
		return obj;
	}

	static void add(lua_State *L, int value) {
		lua_pushinteger(L, value);
	}

	static void add(lua_State *L, const char* value) {
		lua_pushstring(L, value);
	}
	static void add(lua_State *L, std::string value) {
		lua_pushstring(L, value.c_str());
	}

	static void add(lua_State *L, lua_CFunction value) {
		lua_pushcfunction(L, value);
	}

	template<typename KT, typename VT>static void addMember(lua_State *L, KT name, VT value) {
		add(L, name);
		add(L, value);
        lua_rawset(L, -3);
    }
    
	template<typename LT> static void addMember(lua_State *L, const char* name, const std::list<LT> values) {
		lua_pushstring(L, name);
		lua_newtable(L);
		int index = 1;
		for(auto v : values) addMember(L, index++, v);
		lua_rawset(L, -3);
	}
	
	template<typename KT, typename VT> static void addMember(lua_State *L, const char* name, const std::unordered_map<KT, VT> values) {
		add(L, name);
		lua_newtable(L);
		for(auto kv : values) addMember(L, kv.first, kv.second);
		lua_rawset(L, -3);
	}

    static void registerClass(lua_State* L) {
        lua_newtable(L);
        T::addClassMember(L);
        lua_setglobal(L, T::getLuaName());
    }
    
    void bind(lua_State* L) {
        lua_newtable(L);
        lua_pushstring(L, "__object");
        lua_pushlightuserdata(L, (T*)this);
        lua_rawset(L, -3);
        ((T*)this)->addObjectMember(L);
    }
    
    static int getClassMember(lua_State* L, const char* name) {
        if(lua_getglobal(L, T::getLuaName()) != LUA_TNIL) {
            lua_pushstring(L, name);
            return lua_rawget(L, -2);
        }
        return LUA_TNIL;
    }
};

#include <iostream>
class LuaEnv {
	lua_State* _L;
public:
	LuaEnv() {
		_L = luaL_newstate();
		if(_L != nullptr) {
			luaL_openlibs(_L);
			lua_pushcfunction(_L, [](lua_State* L) {
				for(int index = 1; index <= lua_gettop(L); index++) {
					if(lua_isinteger(L, index)) std::cout << lua_tointeger(L, index);
					else if(lua_isstring(L, index)) std::cout << lua_tostring(L, index);
				}
				std::cout << std::endl;
				return 0;
			});
			lua_setglobal(_L, "Print");
		}
	}
    
    ~LuaEnv() {
        if(_L != nullptr) lua_close(_L);
    }
    
	lua_State* get() {
		return _L;
	}

	lua_State* create() {
		return lua_newthread(_L);
	}

	void load(const char* name) {
		if(_L != nullptr) {
			lua_getglobal(_L, "Print");
			int errfunc = lua_gettop(_L);
			luaL_loadfile(_L, name);
			lua_pcall(_L, 0, 0, errfunc);
		}
	}
};

#endif
