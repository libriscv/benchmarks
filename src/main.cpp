#include <libriscv/machine.hpp>
#include <time.h>

timespec time_now()
{
	timespec t;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
	return t;
}
uint64_t nanodiff(timespec start_time, timespec end_time)
{
	return (end_time.tv_sec - start_time.tv_sec) * (long)1e9 + (end_time.tv_nsec - start_time.tv_nsec);
}

int main()
{
	return 0;
}
