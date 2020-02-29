#include "testhelp.hpp"
#include <algorithm>
#include <vector>

static long
run_test(const char* name, int samples, test_func setup, test_func execone)
{
	std::vector<test_result> results;
	for (int i = 0; i < samples; i++)
	{
		setup();
		results.push_back( perform_test<2000>(execone) );
	}
	std::sort(results.begin(), results.end());
	long median = results[results.size() / 2];
	long lowest = results[0];
	long highest = results[results.size()-1];

	printf("%s => median %.3fms  lowest: %.3fms  highest: %.3fms\n",
			name, median / 1e6, lowest / 1e6, highest / 1e6);
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

int main()
{
	run_selftest();
	printf("RISC-V self-test OK\n");
	const int S = 200;
	run_test("libriscv: vector append", S, test_setup, test_1_riscv);
	run_test("lua5.3: table append", S, test_setup, test_1_lua);
	run_test("libriscv: many arguments", S, test_setup, test_2_riscv);
	run_test("lua5.3: many arguments", S, test_setup, test_2_lua);
	run_test("libriscv: integer math", S, test_setup, test_3_riscv);
	run_test("lua5.3: integer math", S, test_setup, test_3_lua);
	return 0;
}
