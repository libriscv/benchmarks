#include <cassert>
#include <cstdio>
#include <cstring>
#include <deque>
extern "C" __attribute__((noreturn)) void _exit(int);
#define PUBLIC_API extern "C" __attribute__((used))

namespace std {
	void __throw_bad_alloc() {
		printf("exception: bad_alloc thrown\n");
		_exit(-1);
	}
}

extern "C"
int __cxa_atexit ( void (*f)(void *), void *p, void *d )
{
	return -1;
}

std::deque<int> vec;
int main()
{
	_exit(666);
}

#define USE_ARRAY
#ifdef USE_ARRAY
#include <array>
std::array<int, 2000*9> array;
static int* setter = array.data();
#endif

PUBLIC_API void test(int arg1)
{
#ifdef USE_ARRAY
	//array[counter++] = arg1;
	*setter++ = arg1;
#else
	vec.emplace_back(arg1);
#endif
}

struct Test {
	int32_t a;
	int64_t b;
};

PUBLIC_API long
test_args(const char* a1, Test& a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	static const char* str = "This is a string";
	if (memcmp(a1, str, __builtin_strlen(str)) == 0
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
extern "C"
long sys_print(const char* data)
{
	return syscall(50, (long) data);
}
extern "C"
long sys_longcall(const char* data, int b, int c, int d, int e, int f, int g)
{
	return syscall(51, (long) data, b, c, d, e, f, g);
}

PUBLIC_API void test_print()
{
	sys_print("This is a string");
}

PUBLIC_API void test_longcall()
{
	sys_longcall("This is a string", 2, 3, 4, 5, 6, 7);
}

PUBLIC_API long selftest(int i, float f)
{
	static int bss = 0;
	printf("i: %d   f: %d   bss: %d\n", i, (int) f, bss);
	assert(i == 1234);
	assert(f == 5678.0);
	assert(bss == 0);
	return 200;
}
