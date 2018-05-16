#include "RoverMotor.h"
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


// ----------------------------------------
// 	Set Node ID here
// ----------------------------------------
unsigned short g_usNodeId_a = 1;
unsigned short g_usNodeId_b = 2;
string g_portName_a         = "USB0";
string g_portName_b         = "USB1";


void* g_pKeyHandle_a = 0;
void* g_pKeyHandle_b = 0;
string g_deviceName;
string g_protocolStackName;
string g_interfaceName;
int g_baudrate = 0;
const string g_programName = "RoverMotor";

void  LogError(string functionName, int p_lResult, unsigned int p_ulErrorCode);
void  PrintHeader();
void  PrintSettings();

int   OpenDevice(unsigned int* p_pErrorCode);
int   CloseDevice(unsigned int* p_pErrorCode);
int   EnableMotor(unsigned int* p_pErrorCode);
int   DisableMotor(unsigned int* p_pErrorCode);

// SINGLETON
RoverMotor* RoverMotor::pinstance = 0;
RoverMotor* RoverMotor::Instance()
{
	if (pinstance == 0) // first time call
	{
		pinstance = new RoverMotor;
	}
	return pinstance;
}
RoverMotor::RoverMotor()
{
}

RoverMotor & RoverMotor::operator=(const RoverMotor & olc)
{
	// shall never get called
	RoverMotor* lc = new RoverMotor();
	return *lc;
}

RoverMotor::~RoverMotor()
{
	delete pinstance;
}


// 
int RoverMotor::openEpos()
{
	int lResult = MMC_FAILED;
	unsigned int ulErrorCode = 0;

	PrintHeader();

	//USB
	g_deviceName        = "EPOS2"; //EPOS version
	g_protocolStackName = "MAXON SERIAL V2"; //MAXON_RS232
	g_interfaceName     = "USB"; //RS232
	g_baudrate          = 1000000; //115200

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

int RoverMotor::disableEpos()
{
	int lResult = MMC_FAILED;
	unsigned int ulErrorCode = 0;

	if((lResult = DisableMotor(&ulErrorCode))!=MMC_SUCCESS)
	{
		LogError("DisableMotor", lResult, ulErrorCode);
	}
	return lResult;
}


int RoverMotor::closeEpos()
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

int RoverMotor::home()
{
	int lResult = MMC_SUCCESS;
	unsigned int ulErrorCode = 0;

	cout << "[RoverMotor.home] Begining Homing:" << endl;
	cout << "[RoverMotor.home] NodeId A = " << g_usNodeId_a << ", NodeID B = " << g_usNodeId_b << endl;

	// --------------------------------------
	// 	Activation
	// --------------------------------------

	if(VCS_ActivatePositionMode(g_pKeyHandle_a, g_usNodeId_a, &ulErrorCode) == 0)
	{
		LogError("VCS_ActivatePositionMode", lResult, ulErrorCode);
		lResult = MMC_FAILED;
		return lResult;
	}
	if(VCS_ActivatePositionMode(g_pKeyHandle_b, g_usNodeId_b, &ulErrorCode) == 0)
	{
		LogError("VCS_ActivatePositionMode", lResult, ulErrorCode);
		lResult = MMC_FAILED;
		return lResult;
	}

	// --------------------------------------
	// 	Homing
	// --------------------------------------
	if(VCS_SetPositionMust(g_pKeyHandle_a, g_usNodeId_a, 0, &ulErrorCode) == 0)
	{
		LogError("VCS_SetPositionMust", lResult, ulErrorCode);
		lResult = MMC_FAILED;
		return lResult;
	}
	if(VCS_SetPositionMust(g_pKeyHandle_b, g_usNodeId_b, 0, &ulErrorCode) == 0)
	{
		LogError("VCS_SetPositionMust", lResult, ulErrorCode);
		lResult = MMC_FAILED;
		return lResult;
	}

	// block thread until position is attained
	int pPositionIs_a = 0;
	int pPositionIs_b = 0;
	const int POSITION_TOLERANCE = 10;
	for (;;)
	{
		// read
		if(VCS_GetPositionIs(g_pKeyHandle_a, g_usNodeId_a, &pPositionIs_a, &ulErrorCode) == 0)
		{
			LogError("VCS_GetPositionIs", lResult, ulErrorCode);
			lResult = MMC_FAILED;
			return lResult;
		}
		if(VCS_GetPositionIs(g_pKeyHandle_b, g_usNodeId_b, &pPositionIs_b, &ulErrorCode) == 0)
		{
			LogError("VCS_GetPositionIs", lResult, ulErrorCode);
			lResult = MMC_FAILED;
			return lResult;
		}
		cout << "[RoverMotor.home] Goal: 0. Pos A: " << pPositionIs_a << ", Pos B: " << pPositionIs_b << endl; 

		// check
		if ((abs(pPositionIs_a) < POSITION_TOLERANCE) && (abs(pPositionIs_b) < POSITION_TOLERANCE))
		 	break;

		// send again
		if(VCS_SetPositionMust(g_pKeyHandle_a, g_usNodeId_a, 0, &ulErrorCode) == 0)
		{
			LogError("VCS_SetPositionMust", lResult, ulErrorCode);
		 	lResult = MMC_FAILED;
		 	return lResult;
		}
		if(VCS_SetPositionMust(g_pKeyHandle_b, g_usNodeId_b, 0, &ulErrorCode) == 0)
		{
		 	LogError("VCS_SetPositionMust", lResult, ulErrorCode);
		 	lResult = MMC_FAILED;
		 	return lResult;
		}
	}
	cout << "[RoverMotor.home] Finished." << endl;

	return lResult;
}

int RoverMotor::readCurrent(short *current)
{

	int lResult = MMC_SUCCESS;
	unsigned int ulErrorCode = 0;

	short c1,c2;
    if (VCS_GetCurrentIs(g_pKeyHandle_a, g_usNodeId_a, &c1, &ulErrorCode) == 0)
	{
		LogError("VCS_GetCurrentIs", lResult, ulErrorCode);
		lResult = MMC_FAILED;
	}
    if (VCS_GetCurrentIs(g_pKeyHandle_b, g_usNodeId_b, &c2, &ulErrorCode) == 0)
	{
		LogError("VCS_GetCurrentIs", lResult, ulErrorCode);
		lResult = MMC_FAILED;
	}

	current[0] = c1;
	current[1] = c2;
}

int RoverMotor::rotate(long speed_motor_a, long speed_motor_b)
{
	int lResult              = MMC_SUCCESS;
	unsigned int ulErrorCode = 0;
	cout << "[RoverMotor.rotate] A: Node id = " << g_usNodeId_a << ", Speed = " << speed_motor_a << endl;
	cout << "[RoverMotor.rotate] B: Node id = " << g_usNodeId_b << ", Speed = " << speed_motor_b << endl;

	// --------------------------------------
	// 	Activation
	// --------------------------------------
	if(VCS_ActivateVelocityMode(g_pKeyHandle_a, g_usNodeId_a, &ulErrorCode) == 0)
	{
		LogError("VCS_ActivateVelocityMode", lResult, ulErrorCode);
		lResult = MMC_FAILED;
		return lResult;
	}
	if(VCS_ActivateVelocityMode(g_pKeyHandle_b, g_usNodeId_b, &ulErrorCode) == 0)
	{
		LogError("VCS_ActivateVelocityMode", lResult, ulErrorCode);
		lResult = MMC_FAILED;
		return lResult;
	}

	// --------------------------------------
	// 	Send command
	// --------------------------------------
	if(VCS_SetVelocityMust(g_pKeyHandle_a, g_usNodeId_a, speed_motor_a, &ulErrorCode) == 0)
	{
		LogError("VCS_SetVelocityMust", lResult, ulErrorCode);
		lResult = MMC_FAILED;
		return lResult;
	}
	if(VCS_SetVelocityMust(g_pKeyHandle_b, g_usNodeId_b, speed_motor_b, &ulErrorCode) == 0)
	{
		LogError("VCS_SetVelocityMust", lResult, ulErrorCode);
		lResult = MMC_FAILED;
		return lResult;
	}

	cout << "[RoverMotor.rotate] Moters are running." << endl;

	return lResult;
}

int RoverMotor::stop()
{
	int lResult              = MMC_SUCCESS;
	unsigned int ulErrorCode = 0;
	cout << "[RoverMotor.stop] A: Node id = " << g_usNodeId_a << ", Stopping. " << endl;
	cout << "[RoverMotor.stop] B: Node id = " << g_usNodeId_b << ", Stopping. " << endl;

	// --------------------------------------
	// 	Activation
	// --------------------------------------
	if(VCS_ActivateVelocityMode(g_pKeyHandle_a, g_usNodeId_a, &ulErrorCode) == 0)
	{
		LogError("VCS_ActivateVelocityMode", lResult, ulErrorCode);
		lResult = MMC_FAILED;
		return lResult;
	}
	if(VCS_ActivateVelocityMode(g_pKeyHandle_b, g_usNodeId_b, &ulErrorCode) == 0)
	{
		LogError("VCS_ActivateVelocityMode", lResult, ulErrorCode);
		lResult = MMC_FAILED;
		return lResult;
	}

	// --------------------------------------
	// 	Send command
	// --------------------------------------
	if(VCS_SetVelocityMust(g_pKeyHandle_a, g_usNodeId_a, 0, &ulErrorCode) == 0)
	{
		LogError("VCS_SetVelocityMust", lResult, ulErrorCode);
		lResult = MMC_FAILED;
		return lResult;
	}
	if(VCS_SetVelocityMust(g_pKeyHandle_b, g_usNodeId_b, 0, &ulErrorCode) == 0)
	{
		LogError("VCS_SetVelocityMust", lResult, ulErrorCode);
		lResult = MMC_FAILED;
		return lResult;
	}

	cout << "[RoverMotor.stop] Moters are stopped." << endl;

	return lResult;
}



void LogError(string functionName, int p_lResult, unsigned int p_ulErrorCode)
{
	cerr << g_programName << ": " << functionName << " failed (result=" << p_lResult << ", errorCode=0x" << std::hex << p_ulErrorCode << ")"<< endl;
}


void PrintSettings()
{
	cout << "default settings:" << endl;
	cout << "node id a           = " << g_usNodeId_a << endl;
	cout << "node id b           = " << g_usNodeId_b << endl;
	cout << "device name         = '" << g_deviceName << "'" << endl;
	cout << "protocal stack name = '" << g_protocolStackName << "'" << endl;
	cout << "interface name      = '" << g_interfaceName << "'" << endl;
	cout << "port name a         = '" << g_portName_a << "'"<< endl;
	cout << "port name b         = '" << g_portName_b << "'"<< endl;
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

	cout << "Open device..." << endl;

	// open driver a
	strcpy(pPortName, g_portName_a.c_str());
	g_pKeyHandle_a = VCS_OpenDevice(pDeviceName, pProtocolStackName, pInterfaceName, pPortName, p_pErrorCode);

	if(g_pKeyHandle_a!=0 && *p_pErrorCode == 0)
	{
		unsigned int lBaudrate = 0;
		unsigned int lTimeout = 0;

		if(VCS_GetProtocolStackSettings(g_pKeyHandle_a, &lBaudrate, &lTimeout, p_pErrorCode)!=0)
		{
			if(VCS_SetProtocolStackSettings(g_pKeyHandle_a, g_baudrate, lTimeout, p_pErrorCode)!=0)
			{
				if(VCS_GetProtocolStackSettings(g_pKeyHandle_a, &lBaudrate, &lTimeout, p_pErrorCode)!=0)
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
		g_pKeyHandle_a = 0;
	}


	// open driver b
	strcpy(pPortName, g_portName_b.c_str());
	g_pKeyHandle_b = VCS_OpenDevice(pDeviceName, pProtocolStackName, pInterfaceName, pPortName, p_pErrorCode);

	if(g_pKeyHandle_b!=0 && *p_pErrorCode == 0)
	{
		unsigned int lBaudrate = 0;
		unsigned int lTimeout = 0;

		if(VCS_GetProtocolStackSettings(g_pKeyHandle_b, &lBaudrate, &lTimeout, p_pErrorCode)!=0)
		{
			if(VCS_SetProtocolStackSettings(g_pKeyHandle_b, g_baudrate, lTimeout, p_pErrorCode)!=0)
			{
				if(VCS_GetProtocolStackSettings(g_pKeyHandle_b, &lBaudrate, &lTimeout, p_pErrorCode)!=0)
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
		g_pKeyHandle_b = 0;
	}

	delete [] pDeviceName;
	delete [] pProtocolStackName;
	delete [] pInterfaceName;
	delete [] pPortName;

	return lResult;
}

int CloseDevice(unsigned int* p_pErrorCode)
{
	int lResult_a = MMC_FAILED;
	int lResult_b = MMC_FAILED;

	*p_pErrorCode = 0;

	cout << "Close device" << endl;

	if(VCS_CloseDevice(g_pKeyHandle_a, p_pErrorCode)!=0 && *p_pErrorCode == 0)
	{
		lResult_a = MMC_SUCCESS;
	}
	if(VCS_CloseDevice(g_pKeyHandle_b, p_pErrorCode)!=0 && *p_pErrorCode == 0)
	{
		lResult_b = MMC_SUCCESS;
	}

	return lResult_a+lResult_b;
}


int EnableMotor(unsigned int* p_pErrorCode)
{
	int lResult = MMC_SUCCESS;
	BOOL oIsFault_a = 0;
	BOOL oIsFault_b = 0;

	if(VCS_GetFaultState(g_pKeyHandle_a, g_usNodeId_a, &oIsFault_a, p_pErrorCode ) == 0)
	{
		LogError("VCS_GetFaultState", lResult, *p_pErrorCode);
		lResult = MMC_FAILED;
	}
	else if(VCS_GetFaultState(g_pKeyHandle_b, g_usNodeId_b, &oIsFault_b, p_pErrorCode ) == 0)
	{
		LogError("VCS_GetFaultState", lResult, *p_pErrorCode);
		lResult = MMC_FAILED;
	}

	if(lResult==0)
	{
		// clear fault if necessary
		if(oIsFault_a)
		{
			stringstream msg;
			msg << "clear fault, node = '" << g_usNodeId_a << "'";
			cout << msg.str() << endl;

			if(VCS_ClearFault(g_pKeyHandle_a, g_usNodeId_a, p_pErrorCode) == 0)
			{
				LogError("VCS_ClearFault", lResult, *p_pErrorCode);
				lResult = MMC_FAILED;
			}
		}
		if(oIsFault_b)
		{
			stringstream msg;
			msg << "clear fault, node = '" << g_usNodeId_b << "'";
			cout << msg.str() << endl;

			if(VCS_ClearFault(g_pKeyHandle_b, g_usNodeId_b, p_pErrorCode) == 0)
			{
				LogError("VCS_ClearFault", lResult, *p_pErrorCode);
				lResult = MMC_FAILED;
			}
		}

		// enable the motor
		if(lResult==0)
		{
			BOOL oIsEnabled = 0;

			if(VCS_GetEnableState(g_pKeyHandle_a, g_usNodeId_a, &oIsEnabled, p_pErrorCode) == 0)
			{
				LogError("VCS_GetEnableState", lResult, *p_pErrorCode);
				lResult = MMC_FAILED;
			}
			if(VCS_GetEnableState(g_pKeyHandle_b, g_usNodeId_b, &oIsEnabled, p_pErrorCode) == 0)
			{
				LogError("VCS_GetEnableState", lResult, *p_pErrorCode);
				lResult = MMC_FAILED;
			}

			if(lResult==0)
			{
				if(!oIsEnabled)
				{
					if(VCS_SetEnableState(g_pKeyHandle_a, g_usNodeId_a, p_pErrorCode) == 0)
					{
						LogError("VCS_SetEnableState", lResult, *p_pErrorCode);
						lResult = MMC_FAILED;
					}
					if(VCS_SetEnableState(g_pKeyHandle_b, g_usNodeId_b, p_pErrorCode) == 0)
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
	BOOL oIsFault_a = 0;
	BOOL oIsFault_b = 0;

	if(VCS_GetFaultState(g_pKeyHandle_a, g_usNodeId_a, &oIsFault_a, p_pErrorCode ) == 0)
	{
		LogError("VCS_GetFaultState", lResult, *p_pErrorCode);
		lResult = MMC_FAILED;
	}
	if(VCS_GetFaultState(g_pKeyHandle_b, g_usNodeId_b, &oIsFault_b, p_pErrorCode ) == 0)
	{
		LogError("VCS_GetFaultState", lResult, *p_pErrorCode);
		lResult = MMC_FAILED;
	}

	if(lResult==0)
	{
		// clear fault if necessary
		if(oIsFault_a)
		{
			stringstream msg;
			msg << "clear fault, node = '" << g_usNodeId_a << "'";
			cout << msg.str() << endl;

			if(VCS_ClearFault(g_pKeyHandle_a, g_usNodeId_a, p_pErrorCode) == 0)
			{
				LogError("VCS_ClearFault", lResult, *p_pErrorCode);
				lResult = MMC_FAILED;
			}
		}
		if(oIsFault_b)
		{
			stringstream msg;
			msg << "clear fault, node = '" << g_usNodeId_b << "'";
			cout << msg.str() << endl;

			if(VCS_ClearFault(g_pKeyHandle_b, g_usNodeId_b, p_pErrorCode) == 0)
			{
				LogError("VCS_ClearFault", lResult, *p_pErrorCode);
				lResult = MMC_FAILED;
			}
		}

		// enable the motor
		if(lResult==0)
		{
			BOOL oIsDisabled = 0;

			if(VCS_GetDisableState(g_pKeyHandle_a, g_usNodeId_a, &oIsDisabled, p_pErrorCode) == 0)
			{
				LogError("VCS_GetDisableState", lResult, *p_pErrorCode);
				lResult = MMC_FAILED;
			}
			if(VCS_GetDisableState(g_pKeyHandle_b, g_usNodeId_b, &oIsDisabled, p_pErrorCode) == 0)
			{
				LogError("VCS_GetDisableState", lResult, *p_pErrorCode);
				lResult = MMC_FAILED;
			}

			if(lResult==0)
			{
				if(!oIsDisabled)
				{
					if(VCS_SetDisableState(g_pKeyHandle_a, g_usNodeId_a, p_pErrorCode) == 0)
					{
						LogError("VCS_SetDisableState", lResult, *p_pErrorCode);
						lResult = MMC_FAILED;
					}
					if(VCS_SetDisableState(g_pKeyHandle_b, g_usNodeId_b, p_pErrorCode) == 0)
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
