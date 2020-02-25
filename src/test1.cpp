#include "luascript.hpp"
#include "testhelp.hpp"
#include "syscalls.hpp"

using namespace riscv;
static Machine<RISCV32>* machine = nullptr;
static State<RISCV32> state;

static Script* luascript = nullptr;

void test_setup()
{
	riscv::verbose_machine = false;
	static auto rvbinary = load_file("../rvprogram/build/rvbinary");
	delete machine;
	machine = new Machine<RISCV32> {rvbinary, 64*1024*1024};

	// the minimum number of syscalls needed for malloc and C++ exceptions
	setup_newlib_syscalls(state, *machine);
	machine->setup_argv({"rvprogram"});

	// run until it stops
	machine->simulate();
	assert(state.exit_code == 666);

	delete luascript;
	luascript = new Script("../luaprogram/script.lua");
}

void test_1_riscv()
{
	machine->vmcall("test", {555});
}
void test_1_lua()
{
	luascript->call("test", 555);
}

void test_2_riscv()
{
	machine->vmcall("test", {111, 222, 333, 444, 555, 666, 777, 888});
}
void test_2_lua()
{
	luascript->call("test", 111, 222, 333, 444, 555, 666, 777, 888);
}
