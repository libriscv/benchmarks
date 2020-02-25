#include "luascript.hpp"
#include <cstdio>

using namespace luabridge;

Script::Script(const std::string& file)
{
    this->state = luaL_newstate();
    luaL_openlibs(this->state);
    // add engine common script
    if (add_file(file) == false)
    {
		throw std::runtime_error("Missing script file");
	}
}
Script::~Script()
{
    if (this->state) lua_close(this->state);
}

bool Script::add_file(std::string file)
{
    // add script file
    if (luaL_loadfile(this->state, file.c_str()) || lua_pcall(this->state, 0, LUA_MULTRET, 0))
    {
        fprintf(stderr, "\nWARNING:\n  %s\n\n", lua_tostring(this->state, -1));
        return false;
    }
    return true;
}
