#include <cassert>
#include <stdio.h>
#include <include/libc.hpp>
#include <deque>
#include <vector>
#include <crc32.hpp>
extern "C" __attribute__((noreturn)) void _exit(int);
#define PUBLIC_API extern "C" __attribute__((used))

namespace std {
	void __throw_bad_alloc() {
		printf("exception: bad_alloc thrown\n");
		_exit(-1);
	}
}

static std::vector<int> vec;
int main(int, const char**)
{
	vec.reserve(1 + 2000);
	_exit(666);
}

#define USE_ARRAY
#ifdef USE_ARRAY
#include <array>
static std::array<int, 2001> array;
static int counter = 0;
#endif

PUBLIC_API void empty_function()
{
}

PUBLIC_API void test(int arg1)
{
#ifdef USE_ARRAY
	array[counter++] = arg1;
#else
	vec.push_back(arg1);
#endif
}

struct Test {
	int32_t a;
	int64_t b;
};

PUBLIC_API long
test_args(uint32_t a1, Test& a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	if (a1 == crc32("This is a string") //__builtin_strcmp("This is a string", a1) == 0
	&& (a2.a == 222 && a2.b == 666) && (a3 == 333) && (a4 == 444) && (a5 == 555)
	&& (a6 == 666) && (a7 == 777) && (a8 == 888)) return 666;
	return -1;
}

PUBLIC_API long test_maffs(int a1, int a2)
{
	int a = a1 + a2;
	int b = a1 - a2;
	int c = a1 * a2;
	int d = a1 / a2;
	return a + b + c + d;
}

#include <include/syscall.hpp>
inline long sys_print(const char* data)
{
	return syscall(50, (long) data);
}
inline long sys_longcall(const char* data, int b, int c, int d, int e, int f, int g)
{
	return syscall(51, (long) data, b, c, d, e, f, g);
}

PUBLIC_API void test_print()
{
	sys_print("This is a string");
}

PUBLIC_API void test_longcall()
{
	for (int i = 0; i < 10; i++)
	sys_longcall("This is a string", 2, 3, 4, 5, 6, 7);
}

#include <microthread.hpp>
PUBLIC_API long test_threads()
{
	microthread::direct(
		[] {
			microthread::yield();
		});
	microthread::yield();
	return 0;
}
PUBLIC_API long test_threads_args()
{
	auto thread = microthread::create(
		[] (int a, int b, int c, int d) -> long {
			microthread::yield();
			return a + b + c + d;
		}, 1, 2, 3, 4);
	microthread::yield();
	return microthread::join(thread);
}

PUBLIC_API long selftest(int i, float f, long long number)
{
	uint64_t value = 555ull / number;
	printf("Value: %lld / %lld == %llu\n", 555ull, number, value);

	static int bss = 0;
	printf("i: %d   f: %d   bss: %d\n", i, (int) f, bss);
	assert(i == 1234);
	assert(f == 5678.0);
	assert(bss == 0);

	// test sending a 64-bit integral value
	uint64_t testvalue = 0x5678000012340000;
	syscall(8, testvalue, testvalue >> 32);

	auto thread = microthread::create(
		[] (int a, int b, long long c) -> long {
			return a + b + c;
		}, 111, 222, 333);
	long retval = microthread::join(thread);
	return retval;
}

uint8_t src_array[300];
uint8_t dst_array[300];
static_assert(sizeof(src_array) == sizeof(dst_array), "!");

PUBLIC_API long test_memcpy()
{
	const char* src = (const char*) src_array;
	char* dest = (char*) dst_array;
	const size_t size = sizeof(dst_array);
	
	for (size_t i = 0; i < size; i++)
		dest[i] = src[i];
	return (long) dest;
}

PUBLIC_API long test_syscall_memcpy()
{
	return (long) memcpy(dst_array, src_array, sizeof(dst_array));
}
