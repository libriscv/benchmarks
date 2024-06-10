#pragma once
#include <chrono>
#include <stdexcept>
#include <string>
#include <time.h>
#include <unistd.h>
#include <vector>
#ifdef WIN32
#include <windows.h>
#endif

using test_func = void(*)();
using mips_func = uint64_t(*)();
using test_result = int64_t;
using time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;

inline auto time_now()
{
#ifdef WIN32
	auto hperfclock = []() -> int64_t {
		LARGE_INTEGER t, f;
		QueryPerformanceCounter(&t);
		QueryPerformanceFrequency(&f);
		return t.QuadPart * 1000000000 / f.QuadPart;
	};
	return time_point(std::chrono::nanoseconds(hperfclock()));
#else
	return std::chrono::high_resolution_clock::now();
#endif
}
inline int64_t nanodiff(time_point start_time, time_point end_time);

template <int ROUNDS = 2000> inline test_result
perform_test(test_func func)
{
	func(); // warmup
	asm("" : : : "memory");
	auto t0 = time_now();
	asm("" : : : "memory");
	for (int i = 0; i < ROUNDS; i++) {
		func();
	}
	asm("" : : : "memory");
	auto t1 = time_now();
	asm("" : : : "memory");
	return nanodiff(t0, t1);
}

template <int ROUNDS = 5000> inline float
measure_mips(const char* name, test_func setup, mips_func func)
{
	setup();
	func();

	uint64_t instructions = 0;
	asm("" : : : "memory");
	auto t0 = time_now();
	asm("" : : : "memory");
	for (int i = 0; i < ROUNDS; i++) {
		instructions += func();
	}
	asm("" : : : "memory");
	auto t1 = time_now();
	asm("" : : : "memory");

	double ts = nanodiff(t0, t1) / 1.0e9;
	double mips = instructions / ts / 1e6;

	printf("%32s\tinstr %lu, time %f, %f mip/s\n", name, instructions, ts, mips);
	return mips;
}

inline std::vector<uint8_t> load_file(const std::string& filename)
{
    size_t size = 0;
    FILE* f = fopen(filename.c_str(), "rb");
    if (f == NULL) throw std::runtime_error("Could not open file: " + filename);

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::vector<uint8_t> result(size);
    if (size != fread(result.data(), 1, size, f))
    {
        fclose(f);
        throw std::runtime_error("Error when reading from file: " + filename);
    }
    fclose(f);
    return result;
}

int64_t nanodiff(time_point start_time, time_point end_time)
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
}
