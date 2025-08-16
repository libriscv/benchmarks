#include "testhelp.hpp"
#include "include/crc32.hpp"
#include <libriscv/machine.hpp>
#include <libriscv/cached_address.hpp>
#include <libriscv/prepared_call.hpp>
#include <cmath>
#include <thread>
#ifndef LUA_DISABLED
#include "luascript.hpp"
#endif
using namespace riscv;
#if RISCV_ARCH == 32
static constexpr int CPUBITS = riscv::RISCV32;
#else
static constexpr int CPUBITS = riscv::RISCV64;
#endif
using machine_t = Machine<CPUBITS>;

static std::vector<uint8_t> rvbinary;
static machine_t* machine = nullptr;
static machine_t* machine_that_holds_execute_segment = nullptr;

static uint32_t test_1_empty_addr = 0x0;
static uint32_t test_1_overhead_args_addr = 0x0;
static uint32_t test_1_syscall_addr[8] = {0x0};
static uint32_t test_1_array_addr = 0x0;
static uint32_t test_1_vector_addr = 0x0;
static uint32_t test_3_fib_addr = 0x0;

#ifndef LUA_DISABLED
static Script* luascript = nullptr;
static luabridge::LuaRef* lua_callable = nullptr;
#endif
extern const char* TEST_BINARY;
static const std::string str("This is a string");
static const struct Test {
	int32_t a = 222;
	int64_t b = 666;
} test;
static riscv::StoredCall<CPUBITS> stored;

template <int W>
void syscall_print(Machine<W>& machine)
{
	// get string_view directly from memory, with max-length
	const auto view = machine.template sysarg<std::string_view>(0);
	if (str != view)
		abort();
}
template <int W>
void syscall_longcall(Machine<W>& machine)
{
	const auto address = machine.template sysarg<address_type<W>>(0);
	// it's a zero-terminated string
	const auto view = machine.memory.memview(address, str.size()+1);
	// compare using memcmp because we have a known length
	if (std::memcmp(str.data(), view.begin(), str.size()+1)) {
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

static std::unique_ptr<PreparedCall<CPUBITS, int(const char*, Test, int, int, int, int, int, int)>> caller_test_2_3 = nullptr;
static std::unique_ptr<PreparedCall<CPUBITS, void()>> caller_test_1_empty;
void test_setup()
{
	if (rvbinary.empty()) {
		rvbinary = load_file(TEST_BINARY);
		// Keep one machine always alive to keep the main execute segment referenced
		machine_that_holds_execute_segment = new machine_t {rvbinary, MachineOptions<CPUBITS>{
			.memory_max = 32*1024*1024,
			.verbose_loader = true,
			.default_exit_function = "fast_exit",
#ifdef RISCV_BINARY_TRANSLATION
			.translate_ignore_instruction_limit = true,
			.translate_use_register_caching = riscv::libtcc_enabled,
			.translate_use_syscall_clobbering_optimization = true,
			.translate_automatic_nbit_address_space = true,
#endif
		}};
		machine_t* m = machine_that_holds_execute_segment;
		assert(m->address_of("empty_function") != 0);
		assert(m->address_of("test_overhead_args") != 0);
		assert(m->address_of("test_array_append") != 0);
		assert(m->address_of("test_vector_append") != 0);
		assert(m->address_of("test_args") != 0);
		assert(m->address_of("test_maffs1") != 0);
		assert(m->address_of("test_maffs2") != 0);
		assert(m->address_of("test_maffs3") != 0);
		assert(m->address_of("test_fib") != 0);
		assert(m->address_of("test_print") != 0);
		assert(m->address_of("test_longcall") != 0);
		assert(m->address_of("test_memcpy") != 0);
		assert(m->address_of("test_syscall_memcpy") != 0);
		test_1_empty_addr = m->address_of("empty_function");
		test_1_overhead_args_addr = m->address_of("test_overhead_args");
		test_1_array_addr = m->address_of("test_array_append");
		test_1_vector_addr = m->address_of("test_vector_append");
		test_1_syscall_addr[0] = m->address_of("test_syscall0");
		test_1_syscall_addr[1] = m->address_of("test_syscall1");
		test_1_syscall_addr[2] = m->address_of("test_syscall2");
		test_1_syscall_addr[3] = m->address_of("test_syscall3");
		test_1_syscall_addr[4] = m->address_of("test_syscall4");
		test_1_syscall_addr[5] = m->address_of("test_syscall5");
		test_1_syscall_addr[6] = m->address_of("test_syscall6");
		test_1_syscall_addr[7] = m->address_of("test_syscall7");
		test_3_fib_addr = m->address_of("test_fib");

		if (m->memory.exit_address() != m->address_of("fast_exit") || m->memory.exit_address() == 0x0) {
			//throw std::runtime_error("'fast_exit' was not found in the RISC-V benchmark program");
			fprintf(stderr, "'fast_exit' was not found in the RISC-V benchmark program\n");
		}

		#define LIVEPATCH_FAST_PATH(func) \
			if (!m->cpu.create_fast_path_function(m->address_of(func))) { \
				throw std::runtime_error("Failed to create fast path for " func); \
			}

		LIVEPATCH_FAST_PATH("measure_mips");
		LIVEPATCH_FAST_PATH("empty_function");
		LIVEPATCH_FAST_PATH("test_overhead_args");
		LIVEPATCH_FAST_PATH("test_array_append");
		LIVEPATCH_FAST_PATH("test_vector_append");
		LIVEPATCH_FAST_PATH("test_args");
		LIVEPATCH_FAST_PATH("test_maffs1");
		LIVEPATCH_FAST_PATH("test_maffs2");
		LIVEPATCH_FAST_PATH("test_maffs3");
		LIVEPATCH_FAST_PATH("test_syscall0");
		LIVEPATCH_FAST_PATH("test_syscall1");
		LIVEPATCH_FAST_PATH("test_syscall2");
		LIVEPATCH_FAST_PATH("test_syscall3");
		LIVEPATCH_FAST_PATH("test_syscall4");
		LIVEPATCH_FAST_PATH("test_syscall5");
		LIVEPATCH_FAST_PATH("test_syscall6");
		LIVEPATCH_FAST_PATH("test_syscall7");
		LIVEPATCH_FAST_PATH("test_fib");
		LIVEPATCH_FAST_PATH("test_print");
		LIVEPATCH_FAST_PATH("test_longcall");
		LIVEPATCH_FAST_PATH("test_threads");
		LIVEPATCH_FAST_PATH("test_threads_args1");
		LIVEPATCH_FAST_PATH("test_threads_args2");
		LIVEPATCH_FAST_PATH("test_memcpy");
		//LIVEPATCH_FAST_PATH("test_syscall_memcpy");
		LIVEPATCH_FAST_PATH("test_memset");
		//LIVEPATCH_FAST_PATH("test_syscall_memset");
		LIVEPATCH_FAST_PATH("test_sieve");
		LIVEPATCH_FAST_PATH("test_taylor");

		delete machine;
		machine = new machine_t {rvbinary, MachineOptions<CPUBITS>{
			.memory_max = 32*1024*1024,
			.default_exit_function = "fast_exit",
#ifdef RISCV_BINARY_TRANSLATION
			.translate_ignore_instruction_limit = true,
			.translate_automatic_nbit_address_space = true,
#endif
		}};

		// the minimum number of syscalls needed for malloc and C++ exceptions
		machine->setup_minimal_syscalls();

		static const uint32_t HEAP_SIZE = 16ull*1024*1024;
		const auto heap_base = machine->memory.mmap_allocate(HEAP_SIZE);
		machine->setup_native_heap(1, heap_base, HEAP_SIZE);
		machine->setup_native_memory(6);
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

		stored.store(*machine, "test_args",
			"This is a string", test,
			333, 444, 555, 666, 777, 888);

		caller_test_1_empty = std::make_unique<PreparedCall<CPUBITS, void()>>(*machine, "empty_function");
		caller_test_2_3 = std::make_unique<PreparedCall<CPUBITS, int(const char*, Test, int, int, int, int, int, int)>>(*machine, "test_args");

		machine->set_max_instructions(5'000'000ULL);
	}

#ifndef LUA_DISABLED
	delete lua_callable;
	delete luascript;
	luascript = new Script("../luaprogram/script.lua");
	lua_callable = new luabridge::LuaRef(luascript->getGlobal("empty_function"));
#endif
}

void test_setup_resume()
{
	test_setup();
	static CachedAddress<CPUBITS> fa;
	machine->vmcall<5'000'000ULL, false>(fa.get(*machine, "resumable_function"));
}

uint64_t riscv_measure_mips()
{
	machine->reset_instruction_counter();
	static CachedAddress<CPUBITS> fa;
	machine->vmcall<500'000'000ULL>(fa.get(*machine, "measure_mips"), 64000);
	return machine->instruction_counter();
}
void bench_fork()
{
	riscv::Machine<CPUBITS> other {*machine, {
		.use_memory_arena = false
	}};
}

void test_1_riscv_vmcall_empty()
{
	caller_test_1_empty->call_with(*machine);
	//machine->vmcall(test_1_empty_addr);
}
template <int N>
void test_1_riscv_args();

template<> void test_1_riscv_args<1>() {
	machine->vmcall(test_1_overhead_args_addr, 1);
}
template<> void test_1_riscv_args<2>() {
	machine->vmcall(test_1_overhead_args_addr, 1, 2);
}
template<> void test_1_riscv_args<3>() {
	machine->vmcall(test_1_overhead_args_addr, 1, 2, 3);
}
template<> void test_1_riscv_args<4>() {
	machine->vmcall(test_1_overhead_args_addr, 1, 2, 3, 4);
}
template<> void test_1_riscv_args<5>() {
	machine->vmcall(test_1_overhead_args_addr, 1, 2, 3, 4, 5);
}
template<> void test_1_riscv_args<6>() {
	machine->vmcall(test_1_overhead_args_addr, 1, 2, 3, 4, 5, 6);
}
template<> void test_1_riscv_args<7>() {
	machine->vmcall(test_1_overhead_args_addr, 1, 2, 3, 4, 5, 6, 7);
}
template<> void test_1_riscv_args<8>() {
	machine->vmcall(test_1_overhead_args_addr, 1, 2, 3, 4, 5, 6, 7, 8);
}

void test_1_riscv_preempt_empty()
{
	machine->reset_instruction_counter();
	machine->preempt(UINT64_MAX, test_1_empty_addr);
}
void test_1_riscv_lookup()
{
	machine->vmcall("empty_function");
}
void test_1_riscv_resume()
{
	machine->simulate(5'000ULL);
}

void test_1_riscv_array()
{
	machine->vmcall(test_1_array_addr, 555);
}
void test_1_riscv_vector()
{
	machine->vmcall(test_1_vector_addr, 555);
}

void test_2_1_riscv_args()
{
	static CachedAddress<CPUBITS> fa;
	int ret = machine->vmcall(fa.get(*machine, "test_args"),
		"This is a string", test, 333, 444, 555, 666, 777, 888);
	if (ret != 666) abort();
}
void test_2_2_riscv_stored()
{
	int ret = stored.vmcall();
	if (ret != 666) abort();
}
void test_2_3_riscv_prepared()
{
	int ret = caller_test_2_3->call_with(*machine,
		"This is a string", test, 333, 444, 555, 666, 777, 888);
	if (ret != 666) abort();
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
	static PreparedCall<CPUBITS, float(float, float, float)> caller(*machine, "test_maffs2");
	caller.call_with(*machine, 3.0f, 3.0f, 3.0f);
}
void test_3_riscv_math3()
{
	static PreparedCall<CPUBITS, float(float, float, float)> caller(*machine, "test_maffs3");
	caller.call_with(*machine, 3.0f, 3.0f, 3.0f);
}
void test_3_riscv_fib()
{
	static PreparedCall<CPUBITS, int(int, int, int)> caller(*machine, "test_fib");
	(void)caller.call_with(*machine, 40, 0, 1);
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

template <int N>
void test_4_riscv_syscall();

template<> void test_4_riscv_syscall<0>() {
	machine->vmcall(test_1_syscall_addr[0]);
}
template<> void test_4_riscv_syscall<1>() {
	machine->vmcall(test_1_syscall_addr[1], 1);
}
template<> void test_4_riscv_syscall<2>() {
	machine->vmcall(test_1_syscall_addr[2], 1, 2);
}
template<> void test_4_riscv_syscall<3>() {
	machine->vmcall(test_1_syscall_addr[3], 1, 2, 3);
}
template<> void test_4_riscv_syscall<4>() {
	machine->vmcall(test_1_syscall_addr[4], 1, 2, 3, 4);
}
template<> void test_4_riscv_syscall<5>() {
	machine->vmcall(test_1_syscall_addr[5], 1, 2, 3, 4, 5);
}
template<> void test_4_riscv_syscall<6>() {
	machine->vmcall(test_1_syscall_addr[6], 1, 2, 3, 4, 5, 6);
}
template<> void test_4_riscv_syscall<7>() {
	machine->vmcall(test_1_syscall_addr[7], 1, 2, 3, 4, 5, 6, 7);
}

void test_4_riscv()
{
	static CachedAddress<CPUBITS> fa;
	machine->vmcall(fa.get(*machine, "test_print"));
}

void test_5_riscv()
{
	static CachedAddress<CPUBITS> fa;
	machine->vmcall(fa.get(*machine, "test_longcall"));
}

void test_6_riscv()
{
	static CachedAddress<CPUBITS> fa;
	machine->vmcall(fa.get(*machine, "test_threads"));
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

void test_8_memcpy_riscv()
{
	static CachedAddress<CPUBITS> fa;
	machine->vmcall(fa.get(*machine, "test_memcpy"));
}
void test_8_memcpy_native_riscv()
{
	static CachedAddress<CPUBITS> fa;
	machine->vmcall(fa.get(*machine, "test_syscall_memcpy"));
}

void test_9_memset_riscv()
{
	static CachedAddress<CPUBITS> fa;
	machine->vmcall(fa.get(*machine, "test_memset"));
}
void test_9_memset_native_riscv()
{
	static CachedAddress<CPUBITS> fa;
	machine->vmcall(fa.get(*machine, "test_syscall_memset"));
}

#ifndef LUA_DISABLED
template <int N>
void test_1_lua_args();

void test_1_lua_empty()
{
	//luascript->call(*lua_callable);
	luascript->call("empty_function");
}
template<> void test_1_lua_args<1>() {
	luascript->call("test_overhead_args", 1);
}
template<> void test_1_lua_args<2>() {
	luascript->call("test_overhead_args", 1, 2);
}
template<> void test_1_lua_args<3>() {
	luascript->call("test_overhead_args", 1, 2, 3);
}
template<> void test_1_lua_args<4>() {
	luascript->call("test_overhead_args", 1, 2, 3, 4);
}
template<> void test_1_lua_args<5>() {
	luascript->call("test_overhead_args", 1, 2, 3, 4, 5);
}
template<> void test_1_lua_args<6>() {
	luascript->call("test_overhead_args", 1, 2, 3, 4, 5, 6);
}
template<> void test_1_lua_args<7>() {
	luascript->call("test_overhead_args", 1, 2, 3, 4, 5, 6, 7);
}
template<> void test_1_lua_args<8>() {
	luascript->call("test_overhead_args", 1, 2, 3, 4, 5, 6, 7, 8);
}
void test_1_lua()
{
	luascript->call("test", 555);
}
void test_2_lua()
{
	auto tab = luascript->new_table();
	tab[0] = 222;
	tab[1] = 666;
	auto ret =
	luascript->retcall("test_args", std::string("This is a string"),
			tab, 333, 444, 555, 666, 777, 888);
	if (int(ret[0]) != 666) abort();
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
void test_4_lua()
{
	luascript->call("test_print");
}

template <int N>
void test_4_lua_syscall();

template<> void test_4_lua_syscall<0>() {
	luascript->call("test_syscall0");
}
template<> void test_4_lua_syscall<1>() {
	luascript->call("test_syscall1", 1);
}
template<> void test_4_lua_syscall<2>() {
	luascript->call("test_syscall2", 1, 2);
}
template<> void test_4_lua_syscall<3>() {
	luascript->call("test_syscall3", 1, 2, 3);
}
template<> void test_4_lua_syscall<4>() {
	luascript->call("test_syscall4", 1, 2, 3, 4);
}
template<> void test_4_lua_syscall<5>() {
	luascript->call("test_syscall5", 1, 2, 3, 4, 5);
}
template<> void test_4_lua_syscall<6>() {
	luascript->call("test_syscall6", 1, 2, 3, 4, 5, 6);
}
template<> void test_4_lua_syscall<7>() {
	luascript->call("test_syscall7", 1, 2, 3, 4, 5, 6, 7);
}

void test_5_lua()
{
	luascript->call("test_longcall");
}
void test_6_lua()
{
	luascript->call("test_threads");
}
void test_7_lua_1()
{
	luascript->call("test_threads_args1");
}
void test_7_lua_2()
{
	luascript->call("test_threads_args2");
}
void test_8_memcpy_lua()
{
	luascript->call("test_memcpy");
}
void test_9_memset_lua()
{
	luascript->call("test_memset");
}
#endif
