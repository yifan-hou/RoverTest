#ifndef _ROVERTEST_CLASS_HEADER_
#define _ROVERTEST_CLASS_HEADER_

#include "TimerLinux.h"

#ifndef MMC_SUCCESS
	#define MMC_SUCCESS 0
#endif

#ifndef MMC_FAILED
	#define MMC_FAILED 1
#endif

#ifndef MMC_MAX_LOG_MSG_SIZE
	#define MMC_MAX_LOG_MSG_SIZE 512
#endif


class RoverTest
{
public:
	/// for singleton implementation 
	static RoverTest* Instance();

	// EPOS
	int openEpos();
	int disableEpos();
	int closeEpos();

	int home();
	int rotate(long speed_motor_a, long speed_motor_b, int time_ms);

private:
	// low level
	int closeFinger(int);

	/// for singleton implementation 
	static RoverTest* pinstance; 
	/// for singleton implementation 
	RoverTest(); 
	/// for singleton implementation 
	RoverTest(const RoverTest&); 
	/// for singleton implementation 
	RoverTest& operator= (const RoverTest&); 
	~RoverTest(); 

	Timer *timer;

};

#endif
