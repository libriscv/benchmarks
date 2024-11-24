#include "luascript.hpp"
#include "testhelp.hpp"
#include <cstdio>
#ifdef LUAU
#include "Luau/Compiler.h"
#endif

using namespace luabridge;

static void nada()
{
	// empty - used to measure syscall overhead
}

static void print(const std::string& text)
{
	if (text != "This is a string") {
		abort();
	}
}

static void longcall(const std::string& str, int, int, int, int, int, int)
{
	if (str != "This is a string") {
		abort();
	}
}

Script::Script(const std::string& file)
{
	this->state = luaL_newstate();
	luaL_openlibs(this->state);
#ifdef LUAU
	//luaL_sandboxthread(this->state);
#else
	lua_sethook(this->state,
	[] (lua_State* state, lua_Debug*) {
		fprintf(stderr, "Lua timeout\n");
		exit(1);
	}, LUA_MASKCOUNT, INT32_MAX);
#endif
	// add engine common script
	if (add_file(file) == false)
	{
		throw std::runtime_error("Missing script file");
	}
	auto gns = getGlobalNamespace(this->state);
	gns.beginNamespace("script")
	.addFunction("args0",    []() -> void { })
	.addFunction("args1",    [](int) -> void { })
	.addFunction("args2",    [](int, int) -> void { })
	.addFunction("args3",    [](int, int, int) -> void { })
	.addFunction("args4",    [](int, int, int, int) -> void { })
	.addFunction("args5",    [](int, int, int, int, int) -> void { })
	.addFunction("args6",    [](int, int, int, int, int, int) -> void { })
	.addFunction("args7",    [](int, int, int, int, int, int, int) -> void { })
	.addFunction("print",    print)
	.addFunction("longcall", longcall)
	.endNamespace();
}
Script::~Script()
{
	if (this->state) lua_close(this->state);
}

bool Script::add_file(std::string file)
{
#ifdef LUAU
	auto vec = load_file(file);
	const std::string source { (const char *)vec.data(), vec.size() };
	const auto bytecode = Luau::compile(source, {
		.optimizationLevel = 2
	});
	if (luau_load(this->state, file.c_str(), bytecode.c_str(), bytecode.size(), 0) || lua_pcall(this->state, 0, LUA_MULTRET, 0))
	{
		fprintf(stderr, "\nWARNING:\n  %s\n\n", lua_tostring(this->state, -1));
		return false;
	}
#else
	// add script file
	if (luaL_loadfile(this->state, file.c_str()) || lua_pcall(this->state, 0, LUA_MULTRET, 0))
	{
		fprintf(stderr, "\nWARNING:\n  %s\n\n", lua_tostring(this->state, -1));
		return false;
	}
#endif
	return true;
}
