#include "ZZJHand.h"
#include "stdio.h"

int main(int argc, char** argv)
{
	ZZJHand *hand = ZZJHand::Instance();

	hand->openEpos();
	hand->getHomePos();
	
	hand->closeEpos();

	return 0;
}