#include "ZZJHand.h"
#include "Definitions.h"

#include <iostream>
#include <string.h>
#include <sstream>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <list>
#include <math.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/times.h>
#include <sys/time.h>


typedef void* HANDLE;
typedef int BOOL;

using namespace std;

void* g_pKeyHandle = 0;
unsigned short g_usNodeId = 1;
string g_deviceName;
string g_protocolStackName;
string g_interfaceName;
string g_portName;
int g_baudrate = 0;

const string g_programName = "HelloEposCmd";


void  LogError(string functionName, int p_lResult, unsigned int p_ulErrorCode);
void  PrintHeader();
void  PrintSettings();

int   OpenDevice(unsigned int* p_pErrorCode);
int   CloseDevice(unsigned int* p_pErrorCode);
int   EnableMotor(unsigned int* p_pErrorCode);
int   DisableMotor(unsigned int* p_pErrorCode);


// SINGLETON
ZZJHand* ZZJHand::pinstance = 0;
ZZJHand* ZZJHand::Instance()
{
	if (pinstance == 0) // first time call
	{
		pinstance = new ZZJHand;
	}
	return pinstance;
}
ZZJHand::ZZJHand()
{
	_home_pos_set = false;
	_grasp_pos_set = false;
}

ZZJHand & ZZJHand::operator=(const ZZJHand & olc)
{
	// shall never get called
	ZZJHand* lc = new ZZJHand();
	return *lc;
}

ZZJHand::~ZZJHand()
{
	delete pinstance;
}


// 
int ZZJHand::openEpos()
{
	int lResult = MMC_FAILED;
	unsigned int ulErrorCode = 0;

	PrintHeader();

	//USB
	g_usNodeId = 1;
	g_deviceName = "EPOS2"; //EPOS version
	g_protocolStackName = "MAXON SERIAL V2"; //MAXON_RS232
	g_interfaceName = "USB"; //RS232
	g_portName = "USB0"; // /dev/ttyS1
	g_baudrate = 1000000; //115200

	PrintSettings();

	if((lResult = OpenDevice(&ulErrorCode))!=MMC_SUCCESS)
	{
		LogError("OpenDevice", lResult, ulErrorCode);
		return lResult;
	}

	// Enable motor
	if((lResult = EnableMotor(&ulErrorCode))!=MMC_SUCCESS)
	{
		LogError("EnableMotor", lResult, ulErrorCode);
		return lResult;
	}

	return lResult;
}

int ZZJHand::disableEpos()
{
	int lResult = MMC_FAILED;
	unsigned int ulErrorCode = 0;

	if((lResult = DisableMotor(&ulErrorCode))!=MMC_SUCCESS)
	{
		LogError("DisableMotor", lResult, ulErrorCode);
	}
	return lResult;
}


int ZZJHand::closeEpos()
{
	int lResult = MMC_FAILED;
	unsigned int ulErrorCode = 0;

	disableEpos();

	if((lResult = CloseDevice(&ulErrorCode))!=MMC_SUCCESS)
	{
		LogError("CloseDevice", lResult, ulErrorCode);
	}
	return lResult;
}


int ZZJHand::getHomePos()
{
	int lResult = MMC_SUCCESS;
	unsigned int ulErrorCode = 0;

	cout << "Getting Zero position, node = " << g_usNodeId << endl;

	// --------------------------------------
	// 	Homing
	// --------------------------------------
	if(VCS_ActivateCurrentMode(g_pKeyHandle, g_usNodeId, &ulErrorCode) == 0)
	{
		LogError("VCS_ActivateCurrentMode", lResult, ulErrorCode);
		lResult = MMC_FAILED;
	}
	else
	{
		// break away
		short current_set = OPEN_FINGER_SIGN*CURRENT_BREAKAWAY_OPENING;
		for (int i = 1; i<5; i++)
		{
			if(VCS_SetCurrentMust(g_pKeyHandle, g_usNodeId, current_set, &ulErrorCode) == 0)
			{
				LogError("VCS_SetCurrentMust", lResult, ulErrorCode);
				lResult = MMC_FAILED;
			}
			usleep(100);
		}

		// Opening
		current_set = OPEN_FINGER_SIGN*CURRENT_OPEN_FINGER;
		if(VCS_SetCurrentMust(g_pKeyHandle, g_usNodeId, current_set, &ulErrorCode) == 0)
		{
			LogError("VCS_SetCurrentMust", lResult, ulErrorCode);
			lResult = MMC_FAILED;
		}

		// block thread until finger is stopped
		int pVelocityIs = 0;
		int zero_count = 0;
		cout << "block thread until finger is stopped." << endl;
		for (;;)
		{
			if(VCS_GetVelocityIs(g_pKeyHandle, g_usNodeId, &pVelocityIs, &ulErrorCode) == 0)
			{
				LogError("VCS_GetVelocityIs", lResult, ulErrorCode);
				lResult = MMC_FAILED;
			}
			// cout << "pVelocityIs: " << pVelocityIs << "count: " << zero_count << endl;
			if (pVelocityIs == 0)
				zero_count++;
			else
				zero_count = 0;

			if (zero_count > 10) break;
		}

		if(VCS_SetCurrentMust(g_pKeyHandle, g_usNodeId, 0, &ulErrorCode) == 0)
		{
			LogError("VCS_SetCurrentMust", lResult, ulErrorCode);
			lResult = MMC_FAILED;
		}				

	}

	// --------------------------------------
	// 	Read Position
	// --------------------------------------
	if(VCS_GetPositionIs(g_pKeyHandle, g_usNodeId, &this->_home_pos, &ulErrorCode) == 0)
	{
		LogError("VCS_GetPositionIs", lResult, ulErrorCode);
		lResult = MMC_FAILED;
	}
	else
	{
		this->_home_pos_set = true;
		cout << "Home position: " << this->_home_pos << endl;
	}
	return lResult;
}

int ZZJHand::openFinger()
{
	int lResult = MMC_SUCCESS;
	unsigned int ulErrorCode = 0;

	cout << "open finger, node = " << g_usNodeId << endl;
	if (this->_home_pos_set == false)
	{
		cerr << "_home_pos_set is false. Remember to run getHomePos() first. " << endl;
		lResult = MMC_FAILED;
		return lResult;
	}

	if(VCS_ActivatePositionMode(g_pKeyHandle, g_usNodeId, &ulErrorCode) == 0)
	{
		LogError("VCS_ActivatePositionMode", lResult, ulErrorCode);
		lResult = MMC_FAILED;
	}
	else
	{
		long pos_set = this->_home_pos - 10*MM2COUNT*OPEN_FINGER_SIGN;
		if(VCS_SetPositionMust(g_pKeyHandle, g_usNodeId, pos_set, &ulErrorCode) == 0)
		{
			LogError("VCS_SetPositionMust", lResult, ulErrorCode);
			lResult = MMC_FAILED;
		}
		// block thread until position is attained
		int pPositionIs = 0;
		for (;;)
		{
			if(VCS_GetPositionIs(g_pKeyHandle, g_usNodeId, &pPositionIs, &ulErrorCode) == 0)
			{
				LogError("VCS_GetPositionIs", lResult, ulErrorCode);
				lResult = MMC_FAILED;
				break;
			}
			// cout << "Open finger Goal: " << pos_set << " Now: " << pPositionIs << endl; 
			if (abs(pPositionIs - pos_set) < POSITION_TOLERANCE) 	break;
			if(VCS_SetPositionMust(g_pKeyHandle, g_usNodeId, pos_set, &ulErrorCode) == 0)
			{
				LogError("VCS_SetPositionMust", lResult, ulErrorCode);
				lResult = MMC_FAILED;
				break;
			}
		}
		cout << "finger opened." << endl;
	}
	return lResult;
}

int ZZJHand::DoFirmGrasp()
{
	int current_set = -OPEN_FINGER_SIGN*CURRENT_FIRM_GRASP;
	_grasp_pos      = closeFinger(current_set);
	_grasp_pos_set  = true;
	return MMC_SUCCESS;
}

int ZZJHand::DoPivotGrasp()
{
	int current_set = -OPEN_FINGER_SIGN*CURRENT_PIVOT_GRASP;
	if (_grasp_pos_set)
	{
		// position control instead
		int lResult = MMC_SUCCESS;
		unsigned int ulErrorCode = 0;

		cout << "Pivot Grasp, node = " << g_usNodeId << endl;

		if(VCS_ActivatePositionMode(g_pKeyHandle, g_usNodeId, &ulErrorCode) == 0)
		{
			LogError("VCS_ActivatePositionMode", lResult, ulErrorCode);
			lResult = MMC_FAILED;
		}
		else
		{
			long pos_set = this->_grasp_pos + OPEN_FINGER_DIST_mm*MM2COUNT*OPEN_FINGER_SIGN;
			if(VCS_SetPositionMust(g_pKeyHandle, g_usNodeId, pos_set, &ulErrorCode) == 0)
			{
				LogError("VCS_SetPositionMust", lResult, ulErrorCode);
				lResult = MMC_FAILED;
			}
			// block thread until position is attained
			int pPositionIs = 0;
			for (;;)
			{
				if(VCS_GetPositionIs(g_pKeyHandle, g_usNodeId, &pPositionIs, &ulErrorCode) == 0)
				{
					LogError("VCS_GetPositionIs", lResult, ulErrorCode);
					lResult = MMC_FAILED;
					break;
				}
				if (abs(pPositionIs - pos_set) < POSITION_TOLERANCE) break;
				cout << "Pivoting Goal: " << pos_set << " Now: " << pPositionIs << endl; 

				if(VCS_SetPositionMust(g_pKeyHandle, g_usNodeId, pos_set, &ulErrorCode) == 0)
				{
					LogError("VCS_SetPositionMust", lResult, ulErrorCode);
					lResult = MMC_FAILED;
					break;
				}
			}
			cout << "Pivot Grasp Done." << endl; 
		}
		return lResult;
	}
	else
	{
		closeFinger(current_set);
		return MMC_SUCCESS;
	}
}

int ZZJHand::closeFinger(int grasp_current)
{
	int lResult = MMC_SUCCESS;
	unsigned int ulErrorCode = 0;

	cout << "Close finger, node = " << g_usNodeId << endl;
	if(VCS_ActivateCurrentMode(g_pKeyHandle, g_usNodeId, &ulErrorCode) == 0)
	{
		LogError("VCS_ActivateCurrentMode", lResult, ulErrorCode);
		lResult = MMC_FAILED;
		return lResult;
	}
	else
	{
		// break away
		short current_set = -OPEN_FINGER_SIGN*CURRENT_BREAKAWAY_CLOSING;
		for(int i=0; i<5; i++)
		{
			if(VCS_SetCurrentMust(g_pKeyHandle, g_usNodeId, current_set, &ulErrorCode) == 0)
			{
				LogError("VCS_SetCurrentMust", lResult, ulErrorCode);
				lResult = MMC_FAILED;
			}
			usleep(100);
		}

		// close up
		current_set = grasp_current;
		if(VCS_SetCurrentMust(g_pKeyHandle, g_usNodeId, current_set, &ulErrorCode) == 0)
		{
			LogError("VCS_SetCurrentMust", lResult, ulErrorCode);
			lResult = MMC_FAILED;
		}


		// block thread until finger is stopped
		int pVelocityIs = 0;
		int zero_count = 0;
		for (;;)
		{
			if(VCS_GetVelocityIs(g_pKeyHandle, g_usNodeId, &pVelocityIs, &ulErrorCode) == 0)
			{
				LogError("VCS_GetVelocityIs", lResult, ulErrorCode);
				lResult = MMC_FAILED;
			}
			if (pVelocityIs == 0)
				zero_count++;
			else
				zero_count = 0;

			if (zero_count > 10) break;
		}
		int current_position = 0;
		if(VCS_GetPositionIs(g_pKeyHandle, g_usNodeId, &current_position, &ulErrorCode) == 0)
		{
			LogError("VCS_GetPositionIs", lResult, ulErrorCode);
			lResult = MMC_FAILED;
		}
		return current_position;
	}
}


void LogError(string functionName, int p_lResult, unsigned int p_ulErrorCode)
{
	cerr << g_programName << ": " << functionName << " failed (result=" << p_lResult << ", errorCode=0x" << std::hex << p_ulErrorCode << ")"<< endl;
}


void PrintSettings()
{
	cout << "default settings:" << endl;
	cout << "node id             = " << g_usNodeId << endl;
	cout << "device name         = '" << g_deviceName << "'" << endl;
	cout << "protocal stack name = '" << g_protocolStackName << "'" << endl;
	cout << "interface name      = '" << g_interfaceName << "'" << endl;
	cout << "port name           = '" << g_portName << "'"<< endl;
	cout << "baudrate            = " << g_baudrate;
	cout << endl;

	const int lineLength = 60;
	for(int i=0; i<lineLength; i++)
	{
		cout << "-";
	}
	cout << endl;
}


int OpenDevice(unsigned int* p_pErrorCode)
{
	int lResult = MMC_FAILED;

	char* pDeviceName = new char[255];
	char* pProtocolStackName = new char[255];
	char* pInterfaceName = new char[255];
	char* pPortName = new char[255];

	strcpy(pDeviceName, g_deviceName.c_str());
	strcpy(pProtocolStackName, g_protocolStackName.c_str());
	strcpy(pInterfaceName, g_interfaceName.c_str());
	strcpy(pPortName, g_portName.c_str());

	cout << "Open device..." << endl;

	g_pKeyHandle = VCS_OpenDevice(pDeviceName, pProtocolStackName, pInterfaceName, pPortName, p_pErrorCode);

	if(g_pKeyHandle!=0 && *p_pErrorCode == 0)
	{
		unsigned int lBaudrate = 0;
		unsigned int lTimeout = 0;

		if(VCS_GetProtocolStackSettings(g_pKeyHandle, &lBaudrate, &lTimeout, p_pErrorCode)!=0)
		{
			if(VCS_SetProtocolStackSettings(g_pKeyHandle, g_baudrate, lTimeout, p_pErrorCode)!=0)
			{
				if(VCS_GetProtocolStackSettings(g_pKeyHandle, &lBaudrate, &lTimeout, p_pErrorCode)!=0)
				{
					if(g_baudrate==(int)lBaudrate)
					{
						lResult = MMC_SUCCESS;
					}
				}
			}
		}
	}
	else
	{
		g_pKeyHandle = 0;
	}

	delete []pDeviceName;
	delete []pProtocolStackName;
	delete []pInterfaceName;
	delete []pPortName;

	return lResult;
}

int CloseDevice(unsigned int* p_pErrorCode)
{
	int lResult = MMC_FAILED;

	*p_pErrorCode = 0;

	cout << "Close device" << endl;

	if(VCS_CloseDevice(g_pKeyHandle, p_pErrorCode)!=0 && *p_pErrorCode == 0)
	{
		lResult = MMC_SUCCESS;
	}

	return lResult;
}


int EnableMotor(unsigned int* p_pErrorCode)
{
	int lResult = MMC_SUCCESS;
	BOOL oIsFault = 0;

	if(VCS_GetFaultState(g_pKeyHandle, g_usNodeId, &oIsFault, p_pErrorCode ) == 0)
	{
		LogError("VCS_GetFaultState", lResult, *p_pErrorCode);
		lResult = MMC_FAILED;
	}

	if(lResult==0)
	{
		// clear fault if necessary
		if(oIsFault)
		{
			stringstream msg;
			msg << "clear fault, node = '" << g_usNodeId << "'";
			cout << msg.str() << endl;

			if(VCS_ClearFault(g_pKeyHandle, g_usNodeId, p_pErrorCode) == 0)
			{
				LogError("VCS_ClearFault", lResult, *p_pErrorCode);
				lResult = MMC_FAILED;
			}
		}

		// enable the motor
		if(lResult==0)
		{
			BOOL oIsEnabled = 0;

			if(VCS_GetEnableState(g_pKeyHandle, g_usNodeId, &oIsEnabled, p_pErrorCode) == 0)
			{
				LogError("VCS_GetEnableState", lResult, *p_pErrorCode);
				lResult = MMC_FAILED;
			}

			if(lResult==0)
			{
				if(!oIsEnabled)
				{
					if(VCS_SetEnableState(g_pKeyHandle, g_usNodeId, p_pErrorCode) == 0)
					{
						LogError("VCS_SetEnableState", lResult, *p_pErrorCode);
						lResult = MMC_FAILED;
					}
				}
			}
		}
	}
	return lResult;
}

int DisableMotor(unsigned int* p_pErrorCode)
{
	int lResult = MMC_SUCCESS;
	BOOL oIsFault = 0;

	if(VCS_GetFaultState(g_pKeyHandle, g_usNodeId, &oIsFault, p_pErrorCode ) == 0)
	{
		LogError("VCS_GetFaultState", lResult, *p_pErrorCode);
		lResult = MMC_FAILED;
	}

	if(lResult==0)
	{
		// clear fault if necessary
		if(oIsFault)
		{
			stringstream msg;
			msg << "clear fault, node = '" << g_usNodeId << "'";
			cout << msg.str() << endl;

			if(VCS_ClearFault(g_pKeyHandle, g_usNodeId, p_pErrorCode) == 0)
			{
				LogError("VCS_ClearFault", lResult, *p_pErrorCode);
				lResult = MMC_FAILED;
			}
		}

		// enable the motor
		if(lResult==0)
		{
			BOOL oIsDisabled = 0;

			if(VCS_GetDisableState(g_pKeyHandle, g_usNodeId, &oIsDisabled, p_pErrorCode) == 0)
			{
				LogError("VCS_GetDisableState", lResult, *p_pErrorCode);
				lResult = MMC_FAILED;
			}

			if(lResult==0)
			{
				if(!oIsDisabled)
				{
					if(VCS_SetDisableState(g_pKeyHandle, g_usNodeId, p_pErrorCode) == 0)
					{
						LogError("VCS_SetDisableState", lResult, *p_pErrorCode);
						lResult = MMC_FAILED;
					}
				}
			}
		}
	}
	return lResult;
}


void PrintHeader()
{
	const int lineLength = 60;
	for(int i=0; i<lineLength; i++)
	{
		cout << "-";
	}
	cout << endl;



	cout << "Epos Command Library Example Program, (c) maxonmotor ag 2014-2017" << endl;

	for(int i=0; i<lineLength; i++)
	{
		cout << "-";
	}
	cout << endl;
}
