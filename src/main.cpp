#include "testhelp.hpp"
#include <algorithm>
#include <vector>

static long
run_test(const char* name, int samples, test_func setup, test_func execone)
{
	std::vector<test_result> results;
	setup();
	for (int i = 0; i < samples; i++)
	{
		auto result = perform_test(execone);
		results.push_back(result);
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
extern void test_1_setup();
extern void test_1_riscv();
extern void test_1_lua();

int main()
{
	const int S = 2000;
	run_test("libriscv: vector append", S, test_1_setup, test_1_riscv);
	run_test("lua5.3: table append", S, test_1_setup, test_1_lua);
	return 0;
}
