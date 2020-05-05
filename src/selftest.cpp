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
	setup_native_heap_syscalls(machine, 1*1024*1024);
	setup_native_memory_syscalls(machine, false);
	setup_native_threads(state.exit_code, machine);
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
	if (state.exit_code != 666) {
		printf(">>> The selftest main function did not return correctly\n");
		printf(">>> The return value was: %d\n", state.exit_code);
		exit(1);
	}
	if (machine.address_of("selftest") == 0) {
		printf(">>> The selftest function is not visible\n");
		exit(1);
	}

	machine.install_syscall_handler(8,
		[] (auto& machine) -> long {
			auto [ll] = machine.template sysargs<uint64_t> ();
			if (ll != 0x5678000012340000) {
				printf("The self-test did not return the correct value\n");
				printf("Got %#lX instead\n", ll);
				exit(1);
			}
			return 0;
		});

	printf("Self-test running test function\n");
	for (int i = 0; i < 10; i++)
	{
		// verify serialization works
		std::vector<uint8_t> mstate;
		machine.serialize_to(mstate);
		machine.deserialize_from(mstate);

		try {
			int ret = machine.vmcall("selftest", 1234, 5678.0, 5ull);
			if (i == 0)
				printf("Output:\n%s", state.output.c_str());
			if (ret != 666) {
				printf("The self-test did not return the correct value\n");
				printf("Got %d instead\n", ret);
				exit(1);
			}
		} catch (riscv::MachineException& me) {
			printf(">>> Machine exception %d: %s (data: %d)\n",
					me.type(), me.what(), me.data());
#ifdef RISCV_DEBUG
			machine.print_and_pause();
#endif
			exit(1);
		}
	}
}
