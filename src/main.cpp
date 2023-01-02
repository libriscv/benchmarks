#include "testhelp.hpp"
#include <algorithm>
#include <vector>
static constexpr bool test_libriscv = true;
static constexpr bool test_lua = true;
static constexpr bool test_sieve = true;

template <size_t TIMES = 1000>
static long
run_test(const char* name, long overhead, int samples, test_func setup, test_func execone)
{
	std::vector<test_result> results;
	for (int i = 0; i < samples; i++)
	{
		setup();
		perform_test<1>(execone); // warmup
		results.push_back( perform_test<TIMES>(execone) );
	}
	std::sort(results.begin(), results.end());
	long median = results[results.size() / 2] / TIMES;
	long lowest = results[0] / TIMES;
	long highest = results[results.size()-1] / TIMES;

	printf("%32s\tmedian %ldns  \t\tlowest: %ldns     \thighest: %ldns\n",
			name, median - overhead, lowest - overhead, highest - overhead);
	return median - overhead;
}

template <size_t TIMES = 50>
static long
slow_test(const char* name, int samples, test_func setup, test_func execone)
{
	std::vector<test_result> results;
	for (int i = 0; i < samples; i++)
	{
		setup();
		perform_test<1>(execone); // warmup
		results.push_back( perform_test<TIMES>(execone) / 1000000 );
	}
	std::sort(results.begin(), results.end());
	long median = results[results.size() / 2] / TIMES;
	long lowest = results[0] / TIMES;
	long highest = results[results.size()-1] / TIMES;

	printf("%32s\tmedian %ldms  \t\tlowest: %ldms     \thighest: %ldms\n",
			name, median, lowest, highest);
	return median;
}

/* TESTS */
extern void run_selftest();
extern void test_setup();
extern uint64_t riscv_measure_mips();
extern void bench_fork();
extern void bench_install_syscall();
extern void test_1_riscv_empty();
extern void test_1_riscv_lookup();
extern void test_1_lua_empty();
extern void test_1_native();
extern void test_1_riscv_direct();
extern void test_1_lua();
extern void test_2_riscv();
extern void test_2_1_riscv();
extern void test_2_2_riscv();
extern void test_2_3_riscv();
extern void test_2_lua();
extern void test_3_riscv();
extern void test_3_riscv_math2();
extern void test_3_riscv_math3();
extern void test_3_riscv_fib();
extern void test_3_riscv_fib40();
extern void test_3_riscv_sieve();
extern void test_3_riscv_taylor();
extern void test_3_lua_math1();
extern void test_3_lua_math2();
extern void test_3_lua_math3();
extern void test_3_lua_fib();
extern void test_3_lua_fib40();
extern void test_3_lua_sieve();
extern void test_3_lua_taylor();
extern void test_4_riscv_syscall();
extern void test_4_riscv();
extern void test_4_lua_syscall();
extern void test_4_lua();
extern void test_5_riscv();
extern void test_5_lua();
extern void test_6_riscv();
extern void test_6_lua();
extern void test_7_riscv_1();
extern void test_7_riscv_2();
extern void test_7_lua_1();
extern void test_7_lua_2();
extern void test_8_memcpy_riscv();
extern void test_8_memcpy_native_riscv();
extern void test_8_memcpy_lua();
extern void test_9_memset_riscv();
extern void test_9_memset_native_riscv();
extern void test_9_memset_lua();

#ifdef RUST_BINARY
const char* TEST_BINARY = "../rvprogram/rustbin/target/riscv32imac-unknown-none-elf/release/rustbin";
#else
//const char* TEST_BINARY = "../rvprogram/build_clang/rvbinary";
const char* TEST_BINARY = "../rvprogram/build/rvbinary";
#endif

#ifdef LUAJIT
#define LUANAME "luajit"
#else
#define LUANAME "lua5.3"
#endif

int main()
{
	const int S = 200;
	printf("* All benchmark results are measured in %dx2000 samples\n", S);
	long riscv_overhead = 0;
	long lua_overhead = 0;

	if constexpr (test_libriscv || test_sieve) {
		run_selftest();
		printf("RISC-V self-test OK\n");
	}
	if constexpr (test_libriscv) {
		measure_mips("libriscv: mips", test_setup, riscv_measure_mips);
		run_test("libriscv: install syscall", 0, S, test_setup, bench_install_syscall);
		riscv_overhead =
			run_test("libriscv: call overhead", 0, S, test_setup, test_1_riscv_empty);
		//run_test("libriscv: lookup overhead", 0, S, test_setup, test_1_riscv_lookup);
		run_test("libriscv: fork", 0, S, test_setup, bench_fork);
	}
	if constexpr (test_lua) {
		lua_overhead =
			run_test(LUANAME ": call overhead", 0, S, test_setup, test_1_lua_empty);
	}
	const long ROH = riscv_overhead;
	const long LOH = lua_overhead;
	printf("\n");
	run_test("native: array append", 0, S, test_setup, test_1_native);
	if constexpr (test_libriscv) {
		run_test("libriscv: array append", ROH, S, test_setup, test_1_riscv_direct);
	}
	if constexpr (test_lua) {
		run_test(LUANAME ": table append", LOH, S, test_setup, test_1_lua);
	}
	printf("\n");
	if constexpr (test_libriscv) {
		run_test("libriscv: many arguments", ROH, S, test_setup, test_2_riscv);
		run_test("libriscv: prepared arguments", ROH, S, test_setup, test_2_1_riscv);
		run_test("libriscv: prepared arguments", ROH, S, test_setup, test_2_2_riscv);
		run_test("libriscv: prepared arguments", ROH, S, test_setup, test_2_3_riscv);
	}
	if constexpr (test_lua) {
		run_test(LUANAME ": many arguments", LOH, S, test_setup, test_2_lua);
	}
	printf("\n");
	if constexpr (test_libriscv) {
		run_test("libriscv: integer math", ROH, S, test_setup, test_3_riscv);
		run_test("libriscv: fp math", ROH, S, test_setup, test_3_riscv_math2);
		run_test("libriscv: exp math", ROH, S, test_setup, test_3_riscv_math3);
		run_test("libriscv: fib(40)", ROH, S, test_setup, test_3_riscv_fib);
		run_test("libriscv: taylor(1K)", ROH, S, test_setup, test_3_riscv_taylor);
	}
	if constexpr (test_lua) {
		run_test(LUANAME ": integer math", LOH, S, test_setup, test_3_lua_math1);
		run_test(LUANAME ": fp math", LOH, S, test_setup, test_3_lua_math2);
		run_test(LUANAME ": exp math", LOH, S, test_setup, test_3_lua_math3);
		run_test(LUANAME ": fib(40)", LOH, S, test_setup, test_3_lua_fib);
		run_test(LUANAME ": taylor(1K)", LOH, S, test_setup, test_3_lua_taylor);
	}
	printf("\n");
	if constexpr (test_libriscv) {
		run_test("libriscv: syscall overhead", ROH, S, test_setup, test_4_riscv_syscall);
		run_test("libriscv: syscall print", ROH, S, test_setup, test_4_riscv);
	}
	if constexpr (test_lua) {
		run_test(LUANAME ": syscall overhead", LOH, S, test_setup, test_4_lua_syscall);
		run_test(LUANAME ": syscall print", LOH, S, test_setup, test_4_lua);
	}
	printf("\n");
	if constexpr (test_libriscv) {
		run_test("libriscv: complex syscall", ROH, S, test_setup, test_5_riscv);
	}
	if constexpr (test_lua) {
		run_test(LUANAME ": complex syscall", LOH, S, test_setup, test_5_lua);
	}
	printf("\n");
	if constexpr (test_libriscv) {
		run_test("libriscv: micro threads", ROH, S, test_setup, test_6_riscv);
	}
	if constexpr (test_lua) {
		run_test(LUANAME ": coroutines", LOH, S, test_setup, test_6_lua);
	}
	printf("\n");
	if constexpr (test_libriscv) {
		run_test("libriscv: micro thread args", ROH, S, test_setup, test_7_riscv_1);
		run_test("libriscv: full thread args", ROH, S, test_setup, test_7_riscv_2);
	}
	if constexpr (test_lua) {
		run_test(LUANAME ": coroutine args", LOH, S, test_setup, test_7_lua_1);
		run_test(LUANAME ": coroutine args", LOH, S, test_setup, test_7_lua_2);
	}
	printf("\n");
	if constexpr (test_libriscv) {
		run_test("libriscv: naive memcpy", ROH, S, test_setup, test_8_memcpy_riscv);
		run_test("libriscv: syscall memcpy", ROH, S, test_setup, test_8_memcpy_native_riscv);
	}
	if constexpr (test_lua) {
		run_test(LUANAME ": memcpy", LOH, S, test_setup, test_8_memcpy_lua);
	}
	if constexpr (test_libriscv) {
		run_test("libriscv: syscall memset", ROH, S, test_setup, test_9_memset_native_riscv);
		run_test("libriscv: naive memset", ROH, S, test_setup, test_9_memset_riscv);
	}
	if constexpr (test_lua) {
		run_test(LUANAME ": memset", LOH, S, test_setup, test_9_memset_lua);
	}
	printf("\n");
	if constexpr (test_sieve) {
		slow_test<10>("libriscv: sieve(10M)", 1, test_setup, test_3_riscv_sieve);
	}
	if constexpr (test_sieve) {
		slow_test<10>(LUANAME ": sieve(10M)", 1, test_setup, test_3_lua_sieve);
	}
	printf("\n");
	return 0;
}
