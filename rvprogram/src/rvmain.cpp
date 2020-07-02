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

#include <include/syscall.hpp>
inline long sys_print(const char* data)
{
	return psyscall(20, data);
}
inline long sys_longcall(const char* data, int b, int c, int d, int e, int f, int g)
{
	return syscall(21, (long) data, b, c, d, e, f, g);
}
inline long sys_nada()
{
	FAST_RETVAL(syscall(22));
}
inline float sys_fmod(float a1, float a2)
{
	return fsyscallf(23, a1, a2);
}
inline float sys_powf(float a1, float a2)
{
	return fsyscallf(24, a1, a2);
}
inline float sys_strcmp(const char* str1, const char* str2)
{
	return psyscall(25, str1, str2);
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
	uint64_t testvalue = 0x5678000012340000;
	syscall(20, testvalue, testvalue >> 32);

	static int changeme = 444;
	microthread::direct(
		[] (auto&, int a, int b, int c, int d) {
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
		[] (auto&) {
		});

	auto thread = microthread::create(
		[] (int a, int b, long long c) -> long {
			return a + b + c;
		}, 111, 222, 333);
	return microthread::join(thread);
}

PUBLIC_API void empty_function()
{
	FAST_RETURN();
}

#include <array>
static std::array<int, 2048> array;
static int counter = 0;

FAST_API void test(int arg1)
{
	//array[counter++] = arg1;
	array.at(counter++) = arg1;
	FAST_RETURN();
}

struct Test {
	int32_t a;
	int64_t b;
};

FAST_API long
test_args(const char* a1, Test& a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	if (sys_strcmp(a1, "This is a string") == 0
	&& (a2.a == 222 && a2.b == 666) && (a3 == 333) && (a4 == 444) && (a5 == 555)
	&& (a6 == 666) && (a7 == 777) && (a8 == 888)) FAST_RETVAL(666);
	FAST_RETVAL(-1);
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

FAST_API void test_syscall()
{
	sys_nada();
	FAST_RETURN();
}
FAST_API void test_print()
{
	sys_print("This is a string");
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
		[] (auto&) {
			microthread::yield();
		});
	microthread::yield();
	FAST_RETURN();
}
static int ttvalue = 0;
FAST_API void test_threads_args1()
{
	microthread::direct(
		[] (auto&, int a, int b, int c, int d) {
			microthread::yield();
			ttvalue = a + b + c + d;
		}, 1, 2, 3, 4);
	microthread::yield();
	FAST_RETURN();
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

uint8_t src_array[300];
uint8_t dst_array[300];
static_assert(sizeof(src_array) == sizeof(dst_array), "!");

FAST_API long test_memcpy()
{
	const char* src = (const char*) src_array;
	char* dest = (char*) dst_array;
	char* dest_end = dest + sizeof(dst_array);

	if ((uintptr_t) dest % 4 == (uintptr_t) src % 4)
	{
		while ((uintptr_t) dest % 4 && dest < dest_end)
			*dest++ = *src++;

		while (dest + 16 <= dest_end) {
			*(uint32_t*) &dest[0] = *(uint32_t*) &src[0];
			*(uint32_t*) &dest[4] = *(uint32_t*) &src[4];
			*(uint32_t*) &dest[8] = *(uint32_t*) &src[8];
			*(uint32_t*) &dest[12] = *(uint32_t*) &src[12];
			dest += 16; src += 16;
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
		sys_print("event_loop: Checking for work\n");
		for (auto& ev : events) ev.handle();
		sys_print("event_loop: Going to sleep!\n");
		asm volatile("ebreak" ::: "memory");
	}
}

PUBLIC_API bool add_work()
{
	Events::Work work {
		[] {
			sys_print("work: Doing some work!\n");
		}
	};
	sys_print("add_work: Adding work\n");
	for (auto& ev : events)
		if (ev.delegate(work)) FAST_RETVAL(true);
	sys_print("add_work: Not adding work this time\n");
	FAST_RETVAL(false);
}
