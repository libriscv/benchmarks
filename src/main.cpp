#include "testhelp.hpp"
#include <algorithm>
#include <vector>

static long
run_test(const char* name, int samples, test_func setup, test_func execone)
{
	static constexpr size_t TIMES = 2000;

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
			name, median, lowest, highest);
	return median;
}

/* TESTS */
extern void run_selftest();
extern void test_setup();
extern void test_1_riscv_empty();
extern void test_1_lua_empty();
extern void test_1_native();
extern void test_1_riscv();
extern void test_1_riscv_direct();
extern void test_1_lua();
extern void test_2_riscv();
extern void test_2_lua();
extern void test_3_riscv();
extern void test_3_lua();
extern void test_4_riscv_syscall();
extern void test_4_riscv();
extern void test_4_lua_syscall();
extern void test_4_lua();
extern void test_5_riscv();
extern void test_5_lua();
extern void test_6_riscv();
extern void test_6_lua();
extern void test_7_riscv();
extern void test_7_lua();
extern void test_8_riscv();
extern void test_8_native_riscv();
extern void test_8_lua();

//const char* TEST_BINARY = "../rvprogram/build_clang/rvbinary";
#ifdef RUST_BINARY
const char* TEST_BINARY = "../rvprogram/rustbin/target/riscv32imac-unknown-none-elf/release/rustbin";
#else
const char* TEST_BINARY = "../rvprogram/build/rvbinary";
#endif

int main()
{
	run_selftest();
	printf("RISC-V self-test OK\n");
	printf("* All benchmark results are measured in 200x2000 samples\n");
	const int S = 200;
	run_test("libriscv: function call", S, test_setup, test_1_riscv_empty);
	run_test("luajit: function call", S, test_setup, test_1_lua_empty);
	printf("\n");
	run_test("native: array append", S, test_setup, test_1_native);
	run_test("libriscv: array append", S, test_setup, test_1_riscv);
	run_test("libriscv: array app. direct", S, test_setup, test_1_riscv_direct);
	run_test("luajit: table append", S, test_setup, test_1_lua);
	printf("\n");
	run_test("libriscv: many arguments", S, test_setup, test_2_riscv);
	run_test("luajit: many arguments", S, test_setup, test_2_lua);
	printf("\n");
	run_test("libriscv: integer math", S, test_setup, test_3_riscv);
	run_test("luajit: integer math", S, test_setup, test_3_lua);
	printf("\n");
	run_test("libriscv: syscall overhead", S, test_setup, test_4_riscv_syscall);
	run_test("libriscv: syscall print", S, test_setup, test_4_riscv);
	run_test("luajit: syscall overhead", S, test_setup, test_4_lua_syscall);
	run_test("luajit: syscall print", S, test_setup, test_4_lua);
	printf("\n");
	run_test("libriscv: complex syscall", S, test_setup, test_5_riscv);
	run_test("luajit: complex syscall", S, test_setup, test_5_lua);
	printf("\n");
	run_test("libriscv: micro threads", S, test_setup, test_6_riscv);
	run_test("luajit: coroutines", S, test_setup, test_6_lua);
	printf("\n");
	run_test("libriscv: micro thread args", S, test_setup, test_7_riscv);
	run_test("luajit: coroutine args", S, test_setup, test_7_lua);
	printf("\n");
	run_test("libriscv: memcpy", S, test_setup, test_8_riscv);
	run_test("libriscv: memcpy native", S, test_setup, test_8_native_riscv);
	run_test("luajit: memcpy", S, test_setup, test_8_lua);
	printf("\n");
	return 0;
}
