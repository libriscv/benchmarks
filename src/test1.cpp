#include "luascript.hpp"
#include "testhelp.hpp"
#include "syscalls.hpp"

using namespace riscv;
static std::vector<uint8_t> rvbinary;
static Machine<RISCV32>* machine = nullptr;
static State<RISCV32> state;

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
	setup_minimal_syscalls(state, *machine);
	setup_native_heap_syscalls(state, *machine);
	machine->throw_on_unhandled_syscall = true;
	machine->install_syscall_handler(50, syscall_print<RISCV32>);
	machine->install_syscall_handler(51, syscall_longcall<RISCV32>);
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
	assert(state.exit_code == 666);

	assert(machine->address_of("test") != 0);
	assert(machine->address_of("test_args") != 0);
	assert(machine->address_of("test_maffs") != 0);
	assert(machine->address_of("test_print") != 0);
	assert(machine->address_of("test_longcall") != 0);

	delete luascript;
	luascript = new Script("../luaprogram/script.lua");
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
		machine->vmcall<0>("test_args", "This is a string", test, 333,
										444, 555, 666, 777, 888);
		if (ret != 666) abort();
	} catch (riscv::MachineException& me) {
		printf(">>> test_2 Machine exception %d: %s (data: %d)\n",
				me.type(), me.what(), me.data());
		machine->print_and_pause();
	}
#else
	int ret =
	machine->vmcall<0>("test_args", "This is a string", test, 333,
									444, 555, 666, 777, 888);
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
	machine->vmcall<0>("test_maffs", 111, 222);
#endif
}
void test_3_lua()
{
	luascript->call("test_maffs", 111, 222);
}

void test_4_riscv()
{
	machine->vmcall<0>("test_print");
}
void test_4_lua()
{
	luascript->call("test_print");
}

void test_5_riscv()
{
	machine->vmcall<0>("test_longcall");
}
void test_5_lua()
{
	luascript->call("test_longcall");
}
