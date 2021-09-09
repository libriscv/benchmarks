#include <include/syscall_helpers.hpp>
#include "luascript.hpp"
#include "testhelp.hpp"
#include "include/crc32.hpp"
#include <libriscv/cached_address.hpp>
#include <cmath>
using namespace riscv;
static constexpr int CPUBITS = riscv::RISCV32;
using machine_t = Machine<CPUBITS>;

static std::vector<uint8_t> rvbinary;
static machine_t* machine = nullptr;
static uint32_t test_1_empty_addr = 0x0;
static uint32_t test_1_syscall_addr = 0x0;
static uint32_t test_1_address = 0x0;
static uint32_t test_3_fib_addr = 0x0;

static Script* luascript = nullptr;
extern const char* TEST_BINARY;
static const std::string str("This is a string");

template <int W>
void syscall_print(Machine<W>& machine)
{
	// get string directly from memory, with max-length
	const auto rvs = machine.template sysarg<riscv::Buffer>(0);
	// for the sequential case just use compare against string_view
	if (rvs.is_sequential()) {
		if (str != rvs.strview())
			abort();
	}
	else if (str != rvs.to_string()) {
		abort();
	}
}
template <int W>
void syscall_longcall(Machine<W>& machine)
{
	const auto address = machine.template sysarg<address_type<W>>(0);
	// compare using memcmp because we have a known length
	if (UNLIKELY(machine.memory.memcmp(str.data(), address, str.size()))) {
		abort();
	}
}
template <int W>
void syscall_nothing(Machine<W>&)
{
}
template <int W>
void syscall_fmod(Machine<W>& machine)
{
	auto& regs = machine.cpu.registers();
	auto& f1 = regs.getfl(0).f32[0];
	auto& f2 = regs.getfl(1).f32[0];
	f1 = std::fmod(f1, f2);
}
template <int W>
void syscall_powf(Machine<W>& machine)
{
	auto& regs = machine.cpu.registers();
	auto& f1 = regs.getfl(0).f32[0];
	auto& f2 = regs.getfl(1).f32[0];
	f1 = std::pow(f1, f2);
}
template <int W>
void syscall_strcmp(Machine<W>& machine)
{
	const auto [a1, a2] =
		machine.template sysargs <riscv::Buffer, address_type<W>> ();
	if (a1.is_sequential()) {
		// this is really fast because the whole thing is sequential
		machine.set_result(
			machine.memory.memcmp(a1.c_str(), a2, a1.size()));
	} else {
		// this is slightly fast because we know one of the lengths
		const std::string str1 = a1.to_string();
		machine.set_result(
			machine.memory.memcmp(str1.c_str(), a2, str1.size()));
	}
}

void test_setup()
{
	if (rvbinary.empty()) {
		rvbinary = load_file(TEST_BINARY);
	}
	delete machine;
	machine = new machine_t {rvbinary, {
		.memory_max = 16*1024*1024,
#ifdef RISCV_BINARY_TRANSLATION
		.block_size_treshold = 5,
		.forward_jumps = true
#endif
	}};

	// the minimum number of syscalls needed for malloc and C++ exceptions
	setup_minimal_syscalls(*machine);

	machine->setup_native_heap(1, 0x40000000, 8*1024*1024);
	machine->setup_native_memory(6, false);
	machine->setup_native_threads(21);

	machine->install_syscall_handler(40, syscall_print<CPUBITS>);
	machine->install_syscall_handler(41, syscall_longcall<CPUBITS>);
	machine->install_syscall_handler(42, syscall_nothing<CPUBITS>);

	machine->install_syscall_handler(43, syscall_fmod<CPUBITS>);
	machine->install_syscall_handler(44, syscall_powf<CPUBITS>);
	machine->install_syscall_handler(45, syscall_strcmp<CPUBITS>);
	machine->setup_argv({"rvprogram"});

	try {
		// run until it stops
		machine->simulate();
	} catch (riscv::MachineException& me) {
		printf(">>> Machine exception %d: %s (data: 0x%lX)\n",
				me.type(), me.what(), me.data());
#ifdef RISCV_DEBUG
		machine->print_and_pause();
#endif
	}
	assert(machine->cpu.reg(10) == 0);

	assert(machine->address_of("empty_function") != 0);
	assert(machine->address_of("test") != 0);
	assert(machine->address_of("test_args") != 0);
	assert(machine->address_of("test_maffs1") != 0);
	assert(machine->address_of("test_maffs2") != 0);
	assert(machine->address_of("test_maffs3") != 0);
	assert(machine->address_of("test_fib") != 0);
	assert(machine->address_of("test_print") != 0);
	assert(machine->address_of("test_longcall") != 0);
	assert(machine->address_of("test_memcpy") != 0);
	assert(machine->address_of("test_syscall_memcpy") != 0);
	test_1_empty_addr = machine->address_of("empty_function");
	test_1_address = machine->address_of("test");
	test_1_syscall_addr = machine->address_of("test_syscall");
	test_3_fib_addr = machine->address_of("test_fib");

	delete luascript;
	luascript = new Script("../luaprogram/script.lua");

	extern void reset_native_tests();
	reset_native_tests();
}

void bench_fork()
{
	riscv::Machine<CPUBITS> other {*machine, {

	}};
}
void bench_install_syscall()
{
	machine->install_syscall_handlers({
		{0,	[] (auto&) {
		}},
		{1,	[] (auto&) {
		}},
		{2,	[] (auto&) {
		}},
		{3,	[] (auto&) {
		}},
		{4,	[] (auto&) {
		}},
		{5,	[] (auto&) {
		}},
		{6,	[] (auto&) {
		}},
		{7,	[] (auto&) {
		}}
	});
}

void test_1_riscv_empty()
{
	machine->vmcall(test_1_empty_addr);
}
void test_1_lua_empty()
{
	luascript->call("empty_function");
}

void test_1_riscv()
{
#ifdef RISCV_DEBUG
	try {
		machine->vmcall("test", 555);
	} catch (riscv::MachineException& me) {
		printf(">>> test_1 Machine exception %d: %s (data: %d)\n",
				me.type(), me.what(), me.data());
		machine->print_and_pause();
	}
#else
	machine->vmcall("test", 555);
#endif
}
void test_1_riscv_direct()
{
	machine->vmcall(test_1_address, 555);
}
void test_1_lua()
{
	luascript->call("test", 555);
}

void test_2_riscv()
{
	static CachedAddress<CPUBITS> fa;
	const struct Test {
		int32_t a = 222;
		int64_t b = 666;
	} test;
#ifdef RISCV_DEBUG
	try {
		int ret =
		machine->vmcall(fa.get(*machine, "test_args"), "This is a string", test,
									333, 444, 555, 666, 777, 888);
		if (ret != 666) abort();
	} catch (riscv::MachineException& me) {
		printf(">>> test_2 Machine exception %d: %s (data: %d)\n",
				me.type(), me.what(), me.data());
		machine->print_and_pause();
	}
#else
	int ret =
	machine->vmcall(fa.get(*machine, "test_args"), "This is a string", test,
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
	static CachedAddress<CPUBITS> fa;
#ifdef RISCV_DEBUG
	try {
		machine->vmcall(fa.get(*machine, "test_maffs1"), 111, 222);
	} catch (riscv::MachineException& me) {
		printf(">>> test_3 Machine exception %d: %s (data: %d)\n",
				me.type(), me.what(), me.data());
		machine->print_and_pause();
	}
#else
	machine->vmcall(fa.get(*machine, "test_maffs1"), 111, 222);
#endif
}
void test_3_riscv_math2()
{
	static CachedAddress<CPUBITS> fa;
	machine->vmcall(fa.get(*machine, "test_maffs2"), 3.0, 3.0, 3.0);
}
void test_3_riscv_math3()
{
	static CachedAddress<CPUBITS> fa;
	machine->vmcall(fa.get(*machine, "test_maffs3"), 3.0, 3.0, 3.0);
}
void test_3_riscv_fib()
{
	machine->vmcall(test_3_fib_addr, 40, 0, 1);
}
void test_3_riscv_sieve()
{
	static CachedAddress<CPUBITS> fa;
	machine->vmcall(fa.get(*machine, "test_sieve"), 10000000);
}
void test_3_riscv_taylor()
{
	static CachedAddress<CPUBITS> fa;
	machine->vmcall(fa.get(*machine, "test_taylor"), 1000);
}

void test_3_lua_math1()
{
	luascript->call("test_maffs1", 111, 222);
}
void test_3_lua_math2()
{
	luascript->call("test_maffs2", 3.0, 3.0, 3.0);
}
void test_3_lua_math3()
{
	luascript->call("test_maffs3", 3.0, 3.0, 3.0);
}
void test_3_lua_fib()
{
	luascript->call("test_fib", 40, 0, 1);
}
void test_3_lua_taylor()
{
	luascript->call("test_taylor", 1000);
}
void test_3_lua_sieve()
{
	luascript->call("test_sieve", 10000000);
}

void test_4_riscv_syscall()
{
	machine->vmcall(test_1_syscall_addr);
}
void test_4_riscv()
{
	static CachedAddress<CPUBITS> fa;
	machine->vmcall(fa.get(*machine, "test_print"));
}
void test_4_lua()
{
	luascript->call("test_print");
}
void test_4_lua_syscall()
{
	luascript->call("test_syscall");
}

void test_5_riscv()
{
	static CachedAddress<CPUBITS> fa;
	machine->vmcall(fa.get(*machine, "test_longcall"));
}
void test_5_lua()
{
	luascript->call("test_longcall");
}

void test_6_riscv()
{
	static CachedAddress<CPUBITS> fa;
	machine->vmcall(fa.get(*machine, "test_threads"));
}
void test_6_lua()
{
	luascript->call("test_threads");
}

void test_7_riscv_1()
{
	static CachedAddress<CPUBITS> fa;
	machine->vmcall(fa.get(*machine, "test_threads_args1"));
}
void test_7_riscv_2()
{
	static CachedAddress<CPUBITS> fa;
	machine->vmcall(fa.get(*machine, "test_threads_args2"));
}
void test_7_lua_1()
{
	luascript->call("test_threads_args1");
}
void test_7_lua_2()
{
	luascript->call("test_threads_args2");
}

void test_8_riscv()
{
	static CachedAddress<CPUBITS> fa;
	machine->vmcall(fa.get(*machine, "test_memcpy"));
}
void test_8_native_riscv()
{
	static CachedAddress<CPUBITS> fa;
	machine->vmcall(fa.get(*machine, "test_syscall_memcpy"));
}
void test_8_lua()
{
	luascript->call("test_memcpy");
}
