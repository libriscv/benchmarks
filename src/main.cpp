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

	printf("%s => median %ldns  lowest: %ldns  highest: %ldns\n",
			name, median, lowest, highest);
	return median;
}

/* TESTS */
extern void run_selftest();
extern void test_setup();
extern void test_1_riscv();
extern void test_1_lua();
extern void test_2_riscv();
extern void test_2_lua();
extern void test_3_riscv();
extern void test_3_lua();
extern void test_4_riscv();
extern void test_4_lua();
extern void test_5_riscv();
extern void test_5_lua();

int main()
{
	run_selftest();
	printf("RISC-V self-test OK\n");
	printf("* All benchmark results are measured in 2000 samples\n");
	const int S = 200;
	run_test("libriscv: vector append", S, test_setup, test_1_riscv);
	run_test("lua5.3: table append", S, test_setup, test_1_lua);
	run_test("libriscv: many arguments", S, test_setup, test_2_riscv);
	run_test("lua5.3: many arguments", S, test_setup, test_2_lua);
	run_test("libriscv: integer math", S, test_setup, test_3_riscv);
	run_test("lua5.3: integer math", S, test_setup, test_3_lua);
	run_test("libriscv: syscall print", S, test_setup, test_4_riscv);
	run_test("lua5.3: syscall print", S, test_setup, test_4_lua);
	run_test("libriscv: complex syscall", S, test_setup, test_5_riscv);
	run_test("lua5.3: complex syscall", S, test_setup, test_5_lua);
	return 0;
}
