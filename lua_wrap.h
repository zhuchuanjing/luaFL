#ifndef LUAWRAP_H
#define LUAWRAP_H

#include "../lua/lua.hpp"
#include <unordered_map>
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
    
    static void addMember(lua_State *L, const char* name, lua_CFunction func) {
        lua_pushstring(L, name);
        lua_pushcfunction(L, func);
        lua_rawset(L, -3);
    }
    
    static void addMember(lua_State *L, const char* name, const char* str) {
        lua_pushstring(L, name);
        lua_pushstring(L, str);
        lua_rawset(L, -3);
    }
    
    static void addMember(lua_State *L, const char* name, int value) {
        lua_pushstring(L, name);
        lua_pushinteger(L, value);
        lua_rawset(L, -3);
    }

	static void addMember(lua_State *L, const char* name, const std::unordered_map<std::string, std::string> values) {
		lua_pushstring(L, name);
		lua_newtable(L);
		for(auto kv : values) addMember(L, kv.first.c_str(), kv.second.c_str());
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

void tickLua();
lua_State* initLua();
lua_State* getLua();
void runLua(const char* name);

#endif
