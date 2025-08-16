#include <libriscv/machine.hpp>
#include "testhelp.hpp"
using namespace riscv;
#if RISCV_ARCH == 32
static constexpr int CPUBITS = riscv::RISCV32;
#else
static constexpr int CPUBITS = riscv::RISCV64;
#endif
using machine_t = Machine<CPUBITS>;
#ifdef RUST_BINARY
const char* TEST_BINARY = "../rvprogram/rustbin/target/riscv32imac-unknown-none-elf/release/rustbin";
#else
const char* TEST_BINARY = (CPUBITS == RISCV32) ? "../rv32-binary" : "../rv64-binary";
#endif

static void setup_selftest_machine(machine_t& machine)
{
	// the minimum number of syscalls needed for malloc and C++ exceptions
	machine.setup_minimal_syscalls();
	machine.setup_native_heap(1, 0x40000000, 8*1024*1024);
	machine.setup_native_memory(6);
	machine.setup_native_threads(21);
	machine.cpu.reset_stack_pointer();
	machine.setup_argv({});
}

void run_selftest()
{
	auto rvbinary = load_file(TEST_BINARY);

#ifdef RISCV_BINARY_TRANSLATION
	std::vector<riscv::MachineTranslationOptions> cc;
	// We don't want to use embeddable code when benchmarking JIT
	if constexpr (!riscv::libtcc_enabled) {
		cc.push_back(riscv::MachineTranslationEmbeddableCodeOptions {});
	}
#endif

	machine_t machine {rvbinary, {
		.memory_max = 32*1024*1024,
#ifdef RISCV_BINARY_TRANSLATION
		.translate_ignore_instruction_limit = true,
		.translate_use_syscall_clobbering_optimization = true,
		.translate_automatic_nbit_address_space = true,
		.cross_compile = cc,
#endif
	}};
	setup_selftest_machine(machine);

#ifdef RISCV_DEBUG
	machine.verbose_instructions = true;
	machine.print_and_pause();
#endif

	printf("Self-test running %s ELF entry at 0x%lX\n",
			TEST_BINARY,
			(long)machine.memory.start_address());
	machine.cpu.reg(10) = 666;
	try {
		// run until it stops
		machine.simulate();
	} catch (riscv::MachineException& me) {
		printf(">>> Machine exception %d: %s (data: 0x%lX)\n",
				me.type(), me.what(), me.data());
#ifdef RISCV_DEBUG
		machine.print_and_pause();
#endif
		exit(1);
	}
	if (machine.cpu.reg(10) != 0) {
		printf(">>> The selftest main function did not return correctly\n");
		printf(">>> The return value was: %d\n", machine.return_value<int>());
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

	long fib = machine.vmcall("test_fib", 40, 0, 1);
	if (fib != 102334155) {
		printf("Wrong fibonacci sequence totals: %ld\n", fib);
		exit(1);
	}

	long primes = machine.vmcall("test_sieve", 10'000'000);
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

	machine.preempt(2000, machine.address_of("add_work"));

	printf("Resuming event loop...!\n");
	machine.simulate<false>(1000);
	machine.simulate<false>(1000);
	machine.preempt(2000, machine.address_of("add_work"));
	machine.simulate<false>(1000);
}
