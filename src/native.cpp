#include <array>
static std::array<int, 200*2001> array;
static int counter = 0;

void reset_native_tests()
{
	counter = 0;
}
void test_1_native()
{
	array[counter++] = 44;
}
