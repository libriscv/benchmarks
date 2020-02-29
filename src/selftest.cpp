#include "syscalls.hpp"
#include "testhelp.hpp"
using namespace riscv;

void run_selftest()
{
	auto rvbinary = load_file("../rvprogram/build/rvbinary");

	riscv::verbose_machine = false;
	Machine<RISCV32> machine {rvbinary, 4*1024*1024};
	State<RISCV32> state;

	// the minimum number of syscalls needed for malloc and C++ exceptions
	setup_minimal_syscalls(state, machine);
	setup_native_heap_syscalls(state, machine);
	machine.setup_argv({"rvprogram"});
	machine.memory.set_exit_address(machine.address_of("fastexit"));

	printf("Self-test running ELF entry at 0x%X\n",
			machine.memory.start_address());
	// run until it stops
	machine.simulate();
	assert(state.exit_code == 666);

	printf("Self-test running test function\n");
	int ret = machine.vmcall("selftest", {1234}, {5678.0});
	printf("Output:\n%s", state.output.c_str());
	assert(ret == 200);
}
