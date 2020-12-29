extern "C" {
#include "MyUNP.h"
}
#include <iostream>

int main(int argc, char* argv[])
{
	std::cout << currtime("%Y-%m-%d %T") << std::endl;
	exit(EXIT_SUCCESS);
}