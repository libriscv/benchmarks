#include <include/syscall_helpers.hpp>
#include "testhelp.hpp"
using namespace riscv;
static constexpr int CPUBITS = riscv::RISCV32;
using machine_t = Machine<CPUBITS>;

//static const char* TEST_BINARY = "../rvprogram/rustbin/target/CPUBITSimac-unknown-none-elf/debug/rustbin";
static const char* TEST_BINARY = "../rvprogram/build/rvbinary";

static void setup_selftest_machine(machine_t& machine, State<CPUBITS>& state)
{
	// the minimum number of syscalls needed for malloc and C++ exceptions
	setup_minimal_syscalls(state, machine);
	machine.setup_native_heap(1, 0x40000000, 8*1024*1024);
	machine.setup_native_memory(6, false);
	machine.setup_native_threads(21);
	machine.setup_argv({});
	machine.memory.set_exit_address(machine.address_of("fastexit"));
}

void run_selftest()
{
	auto rvbinary = load_file(TEST_BINARY);
	State<CPUBITS> state;

	machine_t machine {rvbinary, { .memory_max = 16*1024*1024 }};
	setup_selftest_machine(machine, state);

#ifdef RISCV_DEBUG
	machine.verbose_instructions = true;
	machine.print_and_pause();
#endif

	printf("Self-test running ELF entry at 0x%lX\n",
			(long) machine.memory.start_address());
	machine.cpu.reg(10) = 666;
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
	if (machine.cpu.reg(10) != 0) {
		printf(">>> The selftest main function did not return correctly\n");
		printf(">>> The return value was: %d\n", state.exit_code);
		exit(1);
	}
	if (machine.address_of("selftest") == 0) {
		printf(">>> The selftest function is not visible\n");
		exit(1);
	}

	machine.install_syscall_handler(40,
		[] (auto& machine) {
			auto [ll] = machine.template sysargs<uint64_t> ();
			if (ll != 0x5678000012340000) {
				printf("The self-test did not return the correct value\n");
				printf("Got %#lX instead\n", ll);
				exit(1);
			}
			machine.set_result(0);
		});

	printf("Self-test running test function\n");
	for (int i = 0; i < 10; i++)
	{
		// verify serialization works
		std::vector<uint8_t> mstate;
		machine.serialize_to(mstate);

		// create new blank machine
		machine_t other {rvbinary, { .memory_max = 16*1024*1024 }};
		setup_selftest_machine(other, state);
		// deserialize into new machine (which needs setting up first)
		other.deserialize_from(mstate);

		try {
			int ret = other.vmcall("selftest", 1234, 5678.0, 5ull);
			if (ret != 666) {
				printf("The self-test did not return the correct value\n");
				printf("Got %d instead\n", ret);
				printf("Output: %s\n", state.output.c_str());
				exit(1);
			}
		} catch (riscv::MachineException& me) {
			printf(">>> Machine exception %d: %s (data: %d)\n",
					me.type(), me.what(), me.data());
#ifdef RISCV_DEBUG
			other.print_and_pause();
#endif
			exit(1);
		}
	}

	long fib = machine.vmcall("test_fib", 40, 0, 1);
	if (fib != 102334155) {
		printf("Wrong fibonacci sequence totals: %ld\n", fib);
		exit(1);
	}

	long primes = machine.vmcall("test_sieve", 10000000);
	if (primes != 664579) {
		printf("Wrong number of primes from Sieve: %ld\n", primes);
		exit(1);
	}

	machine.vmcall("test_taylor", 1000);
	double taylor = machine.sysarg<double> (0);
	if (taylor < 3.14 || taylor > 3.15) {
		printf("Wrong taylor series number: %f\n", taylor);
		exit(1);
	}

	// test event loop
	machine.install_syscall_handler(40,
		[] (auto& machine) {
			auto [text] = machine.template sysargs<std::string> ();
			printf("%s", text.c_str());
			machine.set_result(0);
		});
	printf("Calling into event loop...!\n");
	machine.vmcall("event_loop");

	machine.preempt(machine.address_of("add_work"));

	printf("Resuming event loop...!\n");
	machine.simulate(1000);
	machine.simulate(1000);
	machine.preempt(machine.address_of("add_work"));
	machine.simulate(1000);
}
