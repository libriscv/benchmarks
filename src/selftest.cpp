#include "syscalls.hpp"
#include "testhelp.hpp"
using namespace riscv;

void run_selftest()
{
	auto rvbinary = load_file("../rvprogram/build_clang/rvbinary");

	riscv::verbose_machine = false;
	Machine<RISCV32> machine {rvbinary, 4*1024*1024};
	State<RISCV32> state;

	// the minimum number of syscalls needed for malloc and C++ exceptions
	setup_minimal_syscalls(state, machine);
	setup_native_heap_syscalls(state, machine);
	machine.setup_argv({"rvprogram"});
#ifndef RISCV_DEBUG
	machine.memory.set_exit_address(machine.address_of("fastexit"));
#else
	machine.verbose_instructions = true;
	machine.print_and_pause();
#endif

	printf("Self-test running ELF entry at 0x%X\n",
			machine.memory.start_address());
	try {
		// run until it stops
		machine.simulate();
	} catch (riscv::MachineException& me) {
		printf(">>> Machine exception %d: %s (data: %d)\n",
				me.type(), me.what(), me.data());
#ifdef RISCV_DEBUG
		machine.print_and_pause();
#endif
		exit(1);
	}
	assert(state.exit_code == 666);
	assert(machine.address_of("selftest") != 0);

	// verify serialization works
	std::vector<uint8_t> mstate;
	machine.serialize_to(mstate);
	machine.deserialize_from(mstate);

	printf("Self-test running test function\n");
	try {
		int ret = machine.vmcall("selftest", 1234, 5678.0);
		printf("Output:\n%s", state.output.c_str());
		assert(ret == 200);
	} catch (riscv::MachineException& me) {
		printf(">>> Machine exception %d: %s (data: %d)\n",
				me.type(), me.what(), me.data());
#ifdef RISCV_DEBUG
		machine.print_and_pause();
#endif
		exit(1);
	}
}
