#include "luascript.hpp"
#include "testhelp.hpp"
#include "syscalls.hpp"
#include "include/crc32.hpp"

using namespace riscv;
static std::vector<uint8_t> rvbinary;
static Machine<RISCV32>* machine = nullptr;
static State<RISCV32> state;
static uint32_t test_1_empty_addr = 0x0;
static uint32_t test_1_address = 0x0;

static Script* luascript = nullptr;
//static const char* TEST_BINARY = "../rvprogram/build_clang/rvbinary";
static const char* TEST_BINARY = "../rvprogram/build/rvbinary";

template <int W>
long syscall_print(Machine<W>& machine)
{
	const auto address = machine.template sysarg<address_type<W>>(0);
	// get string directly from memory, with max-length
	if (machine.memory.memstring(address) != "This is a string") {
		abort();
	}
	return 0;
}
template <int W>
long syscall_longcall(Machine<W>& machine)
{
	const auto address = machine.template sysarg<address_type<W>>(0);
	// get string directly from memory, with max-length
	if (machine.memory.memstring(address) != "This is a string") {
		abort();
	}
	return 0;
}

void test_setup()
{
	riscv::verbose_machine = false;
	rvbinary = load_file(TEST_BINARY);
	delete machine;
	machine = new Machine<RISCV32> {rvbinary, 4*1024*1024};
#ifndef RISCV_DEBUG
	assert(machine->address_of("fastexit") != 0);
	machine->memory.set_exit_address(machine->address_of("fastexit"));
#endif

	// the minimum number of syscalls needed for malloc and C++ exceptions
#ifdef RISCV_DEBUG
	setup_minimal_syscalls(state, *machine);
#endif
	setup_native_heap_syscalls(*machine, 4*1024*1024);
	setup_native_memory_syscalls(*machine, true);
	setup_native_threads(state.exit_code, *machine);
	machine->install_syscall_handler(20, syscall_print<RISCV32>);
	machine->install_syscall_handler(21, syscall_longcall<RISCV32>);
	machine->setup_argv({"rvprogram"});

	try {
		// run until it stops
		machine->simulate();
	} catch (riscv::MachineException& me) {
		printf(">>> Machine exception %d: %s (data: %d)\n",
				me.type(), me.what(), me.data());
#ifdef RISCV_DEBUG
		machine->print_and_pause();
#endif
	}
	assert(machine->cpu.reg(10) == 666);

	assert(machine->address_of("empty_function") != 0);
	assert(machine->address_of("test") != 0);
	assert(machine->address_of("test_args") != 0);
	assert(machine->address_of("test_maffs") != 0);
	assert(machine->address_of("test_print") != 0);
	assert(machine->address_of("test_longcall") != 0);
	assert(machine->address_of("test_memcpy") != 0);
	assert(machine->address_of("test_syscall_memcpy") != 0);
	test_1_empty_addr = machine->address_of("empty_function");
	test_1_address = machine->address_of("test");

	delete luascript;
	luascript = new Script("../luaprogram/script.lua");
	
	extern void reset_native_tests();
	reset_native_tests();
}

void test_1_riscv_empty()
{
	machine->vmcall<0>(test_1_empty_addr);
}
void test_1_lua_empty()
{
	luascript->call("empty_function");
}

void test_1_riscv()
{
#ifdef RISCV_DEBUG
	try {
		machine->vmcall<0>("test", 555);
	} catch (riscv::MachineException& me) {
		printf(">>> test_1 Machine exception %d: %s (data: %d)\n",
				me.type(), me.what(), me.data());
		machine->print_and_pause();
	}
#else
	machine->vmcall<0>("test", 555);
#endif
}
void test_1_riscv_direct()
{
	machine->vmcall<0>(test_1_address, 555);
}
void test_1_lua()
{
	luascript->call("test", 555);
}

void test_2_riscv()
{
	const struct Test {
		int32_t a = 222;
		int64_t b = 666;
	} test;
#ifdef RISCV_DEBUG
	try {
		int ret =
		machine->vmcall("test_args", crc32("This is a string"), test,
									333, 444, 555, 666, 777, 888);
		if (ret != 666) abort();
	} catch (riscv::MachineException& me) {
		printf(">>> test_2 Machine exception %d: %s (data: %d)\n",
				me.type(), me.what(), me.data());
		machine->print_and_pause();
	}
#else
	int ret =
	machine->vmcall("test_args", crc32("This is a string"), test,
								333, 444, 555, 666, 777, 888);
	if (ret != 666) abort();
#endif
}
void test_2_lua()
{
	auto tab = luascript->new_table();
	tab[0] = 222;
	tab[1] = 666;
	auto ret =
	luascript->retcall("test_args", "This is a string",
			tab, 333, 444, 555, 666, 777, 888);
	if ((int) ret != 666) abort();
}

void test_3_riscv()
{
#ifdef RISCV_DEBUG
	try {
		machine->vmcall<0>("test_maffs", 111, 222);
	} catch (riscv::MachineException& me) {
		printf(">>> test_3 Machine exception %d: %s (data: %d)\n",
				me.type(), me.what(), me.data());
		machine->print_and_pause();
	}
#else
	machine->vmcall("test_maffs", 111, 222);
#endif
}
void test_3_lua()
{
	luascript->call("test_maffs", 111, 222);
}

void test_4_riscv()
{
	machine->vmcall("test_print");
}
void test_4_lua()
{
	luascript->call("test_print");
}

void test_5_riscv()
{
	machine->vmcall("test_longcall");
}
void test_5_lua()
{
	luascript->call("test_longcall");
}

void test_6_riscv()
{
	machine->vmcall("test_threads");
}
void test_6_lua()
{
	luascript->call("test_threads");
}

void test_7_riscv()
{
	machine->vmcall("test_threads_args");
}
void test_7_lua()
{
	luascript->call("test_threads_args");
}

void test_8_riscv()
{
	machine->vmcall<1'000'000>("test_memcpy");
}
void test_8_native_riscv()
{
	machine->vmcall<1'000'000>("test_syscall_memcpy");
}
void test_8_lua()
{
	luascript->call("test_memcpy");
}
