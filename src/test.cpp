#include <iostream>
#include "RoverTest.h"
#include "Definitions.h"

using namespace std;

int main(int argc, char** argv)
{
	RoverTest *rover = RoverTest::Instance();

	rover->openEpos();

	cout << "Press ENTER to begin homing.." << endl;	
	getchar();

	if(rover->home() != MMC_SUCCESS)
	{
		rover->closeEpos();
		return -1;
	} 

	cout << "Press ENTER to begin Rotation.." << endl;	
	getchar();

	if(rover->rotate(300, 300, 2000) != MMC_SUCCESS)
	{
		rover->closeEpos();
		return -1;
	} 
	
	rover->closeEpos();
	
	
	return 0;
}