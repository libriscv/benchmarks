#include <vector>
extern "C" void _exit(int);

static std::vector<int> array;
int main()
{
	array.reserve(2000);
	_exit(666);
}

extern "C"
void test(int arg1)
{
	array.push_back(arg1);
}

extern "C"
void test_args(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
	array.push_back(a1);
	array.push_back(a2);
	array.push_back(a3);
	array.push_back(a4);
	array.push_back(a5);
	array.push_back(a6);
	array.push_back(a7);
	array.push_back(a8);
}
