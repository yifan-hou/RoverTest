#include "RoverTest.h"
#include "stdio.h"

int main(int argc, char** argv)
{
	RoverTest *rover = RoverTest::Instance();

	rover->openEpos();
	rover->closeEpos();

	return 0;
}