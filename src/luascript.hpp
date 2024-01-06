#pragma once
#include <stdexcept>
#include <string>
extern "C" {
#if defined(LUAU)
#include "lua.h"
#include "lualib.h"
#elif defined(LUAJIT)
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lua.h>
#include <luajit-2.1/lualib.h>
#else
#include <lua5.3/lauxlib.h>
#include <lua5.3/lua.h>
#include <lua5.3/lualib.h>
#endif
}
#include <LuaBridge/LuaBridge.h>

class Script {
public:
    // call any script function, with any parameters
    // returns lua reference
    template <typename... Args>
    inline luabridge::LuaResult retcall(const std::string& name, Args&&...);
    // returns nada
    template <typename... Args>
    inline void call(const std::string& name, Args&&...);

    // create Lua object
    auto ref() const { return luabridge::LuaRef(this->state); }
    auto new_table() const { return luabridge::newTable(this->state); }

    bool add_file(std::string file);

    Script(const std::string&);
    ~Script();

private:
    lua_State* state;
};

template <typename... Args>
inline luabridge::LuaResult Script::retcall(const std::string& name, Args&&... args)
{
    auto lref = luabridge::getGlobal(this->state, name.c_str());
    if (lref.isNil()) throw std::runtime_error(std::string("LUA: '") + name + "' did not exist");
    auto ret = lref(std::forward<Args>(args)...);
    return ret;
}

template <typename... Args>
inline void Script::call(const std::string& name, Args&&... args)
{
    auto lref = luabridge::getGlobal(this->state, name.c_str());
    if (lref.isNil()) throw std::runtime_error(std::string("LUA: '") + name + "' did not exist");
    lref(std::forward<Args>(args)...);
}
