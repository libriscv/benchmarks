#include <cassert>
#include <cstdio>
#include <deque>
extern "C" __attribute__((noreturn)) void _exit(int);
#define PUBLIC_API extern "C" __attribute__((used))

namespace std {
	void __throw_bad_alloc() {
		_exit(-1);
	}
}

std::deque<int> vec;
int main()
{
	_exit(666);
}

//#define USE_ARRAY
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

PUBLIC_API void
test_args(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
#ifdef USE_ARRAY
	*setter++ = a1;
	*setter++ = a2;
	*setter++ = a3;
	*setter++ = a4;
	*setter++ = a5;
	*setter++ = a6;
	*setter++ = a7;
	*setter++ = a8;
#else
	vec.push_back(a1);
	vec.push_back(a2);
	vec.push_back(a3);
	vec.push_back(a4);
	vec.push_back(a5);
	vec.push_back(a6);
	vec.push_back(a7);
	vec.push_back(a8);
#endif
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
long sys_print(const void* data, size_t len)
{
	return syscall(50, (long) data, len);
}

PUBLIC_API void test_print()
{
	const char text[] = "This is a string";
	sys_print(text, sizeof(text)-1);
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
