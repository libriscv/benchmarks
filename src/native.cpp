#include <array>
static std::array<int, 2001> array;
static int counter = 0;

void test_1_native()
{
	array[counter++] = 44;
}
