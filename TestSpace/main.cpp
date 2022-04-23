#include <iostream>
#include <unordered_map>
#include <vector>
#include <memory>
#include <utility>

#include "tinyxml.h"

struct sTest
{
	int Z;
	int Y;
	int X;
};

template <typename T>
void Test(decltype(&T::X) t)
{
	return;
}

int main()
{
	std::cout << sizeof(sTest::X) << std::endl;
	std::cout << offsetof(sTest, X) << std::endl;
	std::cout << &sTest::X << std::endl;
}