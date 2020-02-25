#include "luascript.hpp"
#include "testhelp.hpp"
#include "syscalls.hpp"

using namespace riscv;
static Machine<RISCV32>* machine = nullptr;
static State<RISCV32> state;

static Script* luascript = nullptr;

void test_1_setup()
{
	riscv::verbose_machine = false;
	static auto rvbinary = load_file("../rvprogram/build/rvbinary");
	delete machine;
	machine = new Machine<RISCV32> {rvbinary};

	// the minimum number of syscalls needed for malloc and C++ exceptions
	setup_newlib_syscalls(state, *machine);
	machine->setup_argv({"rvprogram"});

	// run until it stops
	machine->simulate();
	assert(state.exit_code == 666);

	luascript = new Script("../luaprogram/script.lua");
}
void test_1_riscv()
{
	static std::vector<uint32_t> args = {555};
	machine->vmcall("test", args, false);
}
void test_1_lua()
{
	luascript->call("test");
}
