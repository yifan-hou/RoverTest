#include "ZZJHand.h"
#include "stdio.h"
#include "Definitions.h"


int main(int argc, char** argv)
{
	ZZJHand *hand = ZZJHand::Instance();

	hand->openEpos();

	hand->getHomePos();

	
	getchar();
	if(hand->DoFirmGrasp() != MMC_SUCCESS)
	{
		hand->closeEpos();
		return -1;
	} 

	getchar();
	// if(hand->openFinger() != MMC_SUCCESS)
	// {
	// 	hand->closeEpos();
	// 	return -1;
	// } 
	
	// getchar();
	if(hand->DoPivotGrasp() != MMC_SUCCESS)
	{
		hand->closeEpos();
		return -1;
	} 

	getchar();

	if(hand->openFinger() != MMC_SUCCESS)
	{
		hand->closeEpos();
		return -1;
	} 
	
	hand->closeEpos();
	
	
	return 0;
}