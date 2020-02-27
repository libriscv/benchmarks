#include <vector>
extern "C" void _exit(int);
extern "C" void fastexit(int) __attribute__((noreturn));

asm(".global fastexit\n"
	"fastexit:\n"
	"ebreak\n");

std::vector<int> vec;
int main()
{
	vec.reserve(2000 * 9);
	_exit(666);
}

#define USE_ARRAY
#ifdef USE_ARRAY
#include <array>
std::array<int, 2000*8> array;
static int* setter = array.data();
#endif

extern "C"
void test(int arg1)
{
#ifdef USE_ARRAY
	//array[counter++] = arg1;
	*setter++ = arg1;
#else
	vec.emplace_back(arg1);
#endif
}

extern "C"
void test_args(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
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

extern "C"
long test_maffs(int a1, int a2)
{
	int a = a1 + a2;
	int b = a1 - a2;
	int c = a1 * a2;
	int d = a1 / a2;
	return a + b + c + d;
}
