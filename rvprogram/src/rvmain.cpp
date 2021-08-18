#include <cassert>
#include <include/libc.hpp>
#include <crc32.hpp>
#include <microthread.hpp>
#include <stdio.h>
extern "C" __attribute__((noreturn)) void fastexit(int);
#define FAST_RETURN() { asm volatile("ebreak"); __builtin_unreachable(); }
#define FAST_RETVAL(x) \
		{register long __a0 asm("a0") = (long) (x); \
		asm volatile("ebreak" :: "r"(__a0)); __builtin_unreachable(); }

#if defined(__clang__)
#define NORMAL_FUNCTIONS
#endif
#define PUBLIC_API extern "C" __attribute__((used))

#ifdef NORMAL_FUNCTIONS
#define FAST_API extern "C" __attribute__((used))
#else
#define FAST_API extern "C" __attribute__((used, naked))
#endif

#define PRINT(x) sys_print(x, __builtin_strlen(x))

#include <include/syscall.hpp>
inline long sys_print(const char* data, size_t len)
{
	return psyscall(40, data, len);
}
inline long sys_longcall(const char* data, int b, int c, int d, int e, int f, int g)
{
	return syscall(41, (long) data, b, c, d, e, f, g);
}
inline long sys_nada()
{
	return syscall(42);
}
inline float sys_fmod(float a1, float a2)
{
	return fsyscallf(43, a1, a2);
}
inline float sys_powf(float a1, float a2)
{
	return fsyscallf(44, a1, a2);
}
inline int sys_strcmp(const char* str1, size_t len1, const char* str2)
{
	return psyscall(45, str1, len1, str2);
}

int test_main(int argc, char**)
{
	// This only gets run by the benchmarking machines
	if (argc > 0)
	{
		static const char test_string[] = "1234";
		int cmp = sys_strcmp(test_string, sizeof(test_string)-1, "1234");
		assert(cmp == 0);
	}
	return 0;
}

PUBLIC_API long selftest(int i, float f, long long number)
{
	uint64_t value = 555ull / number;
	static int bss = 0;
	assert(i == 1234);
	assert(f == 5678.0);
	assert(bss == 0);
	assert(value == 111ull);
	assert(bss == 0);

	// test sending a 64-bit integral value
	static const uint64_t testvalue = 0x5678000012340000;
	assert(syscall(40, testvalue, testvalue >> 32) == 0);

	static int changeme = 444;
	microthread::direct(
		[] (int a, int b, int c, int d) {
			assert(a == 1);
			assert(b == 2);
			assert(c == 3);
			assert(d == 4);
			microthread::yield();
			changeme = 555;
		}, 1, 2, 3, 4);
	microthread::yield();
	if (changeme != 555) {
		fprintf(stderr,
			"The microthread direct() call did not fully complete\n");
		abort();
	}
	microthread::direct(
		[] { });

	auto thread = microthread::create(
		[] (int a, int b, long long c) -> long {
			return a + b + c;
		}, 111, 222, 333);
	return microthread::join(thread);
}

FAST_API void empty_function()
{
	FAST_RETURN();
}

static const char str[] = "This is a string";

#include <array>
static std::array<int, 2048> array;
static int counter = 0;

PUBLIC_API void test(int arg1)
{
	array[counter++] = arg1;
	counter %= 2048;
	//array.at(counter++) = arg1;
}

struct Test {
	int32_t a;
	int64_t b;
};

PUBLIC_API long
test_args(const char* a1, Test& a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	if (sys_strcmp(str, sizeof(str)-1, a1) == 0
	&& (a2.a == 222 && a2.b == 666) && (a3 == 333) && (a4 == 444) && (a5 == 555)
	&& (a6 == 666) && (a7 == 777) && (a8 == 888)) return 666;
	return -1;
}

#include <cmath>
FAST_API long test_maffs1(int a1, int a2)
{
	int a = a1 + a2;
	int b = a1 - a2;
	int c = a1 * a2;
	int d = a1 / a2;
	FAST_RETVAL(a + b + c + d);
}
FAST_API float test_maffs2(float arg1, float arg2, float arg3)
{
	FAST_RETVAL((arg1 * arg1 * arg3) / (arg2 * arg2 * arg3) + sys_fmod(arg1, arg3));
}
FAST_API float test_maffs3(float arg1, float arg2, float arg3)
{
	FAST_RETVAL(sys_powf(sys_powf(sys_powf(arg1, arg2), 1.0 / arg3), 1.0 / arg3));
}
PUBLIC_API int32_t test_fib(int32_t n, int32_t acc = 0, int32_t prev = 1)
{
	if (n < 1)
		return acc;
	else
		return test_fib(n - 1, prev + acc, acc);
}
PUBLIC_API double test_taylor(int n)
{
	double sum = 1.0;
	for (int i = 1; i < n;) {
		sum += -1.0 / (2.0 * (i++) + 1.0);
		sum +=  1.0 / (2.0 * (i++) + 1.0);
		sum += -1.0 / (2.0 * (i++) + 1.0);
		sum +=  1.0 / (2.0 * (i++) + 1.0);
	}
	return 4.0 * sum;
}
PUBLIC_API long test_sieve(const long N)
{
	bool prime[10'000'000];
	memset(prime, true, sizeof(prime));
	long nprimes = 0;
	for (long n = 2; n < N; n++)
	{
		if (UNLIKELY(prime[n])) {
			nprimes += 1;
			for (long i = 2*n; i < N; i += n)
				prime[i] = false;
		}
	}
	return nprimes;
}

FAST_API void test_syscall()
{
	sys_nada();
	FAST_RETURN();
}
FAST_API void test_print()
{
	PRINT("This is a string");
	FAST_RETURN();
}

FAST_API void test_longcall()
{
	for (int i = 0; i < 10; i++)
		sys_longcall("This is a string", 2, 3, 4, 5, 6, 7);
	FAST_RETURN();
}

FAST_API void test_threads()
{
	microthread::direct(
		[] () {
			microthread::yield();
		});
	microthread::yield();
	FAST_RETURN();
}
static int ttvalue = 0;
PUBLIC_API void test_threads_args1()
{
	microthread::direct(
		[] (int a, int b, int c, int d) {
			microthread::yield();
			ttvalue = a + b + c + d;
		}, 1, 2, 3, 4);
	microthread::yield();
}
PUBLIC_API long test_threads_args2()
{
	auto thread = microthread::create(
		[] (int a, int b, int c, int d) -> long {
			microthread::yield();
			return a + b + c + d;
		}, 1, 2, 3, 4);
	microthread::yield();
	FAST_RETVAL(microthread::join(thread));
}

static uint8_t src_array[300] __attribute__((aligned(16)));
static uint8_t dst_array[300] __attribute__((aligned(16)));
static_assert(sizeof(src_array) == sizeof(dst_array), "!");

FAST_API long test_memcpy()
{
	const char* src = (const char*) src_array;
	char* dest = (char*) dst_array;
	char* dest_end = dest + sizeof(dst_array);
	using T = uint64_t;
	constexpr size_t S = sizeof(T);

	if ((uintptr_t) dest % S == (uintptr_t) src % S)
	{
		while ((uintptr_t) dest % S && dest < dest_end)
			*dest++ = *src++;

		while (dest + 4*S <= dest_end) {
			*(T*) &dest[0*S] = *(T*) &src[0*S];
			*(T*) &dest[1*S] = *(T*) &src[1*S];
			*(T*) &dest[2*S] = *(T*) &src[2*S];
			*(T*) &dest[3*S] = *(T*) &src[3*S];
			dest += 4*S; src += 4*S;
		}

		while (dest + 4 <= dest_end) {
			*(uint32_t*) dest = *(uint32_t*) src;
			dest += 4; src += 4;
		}
	}

	while (dest < dest_end)
		*dest++ = *src++;
	FAST_RETVAL(dest);
}

FAST_API long test_syscall_memcpy()
{
	FAST_RETVAL(memcpy(dst_array, src_array, sizeof(dst_array)));
}


#include <include/event_loop.hpp>
static std::array<Events, 2> events;

PUBLIC_API void event_loop()
{
	while (true) {
		PRINT("event_loop: Checking for work\n");
		for (auto& ev : events) ev.handle();
		PRINT("event_loop: Going to sleep!\n");
		asm volatile("ebreak" ::: "memory");
	}
}

PUBLIC_API bool add_work()
{
	Events::Work work {
		[] {
			PRINT("work: Doing some work!\n");
		}
	};
	PRINT("add_work: Adding work\n");
	for (auto& ev : events)
		if (ev.delegate(work)) FAST_RETVAL(true);
	PRINT("add_work: Not adding work this time\n");
	FAST_RETVAL(false);
}
