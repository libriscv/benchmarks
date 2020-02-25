#include <vector>
extern "C" void _exit(int);

static std::vector<int> array;
int main()
{
	_exit(666);
}

extern "C"
void test(int arg1)
{
	array.push_back(arg1);
}
