#ifndef _RoverMotor_CLASS_HEADER_
#define _RoverMotor_CLASS_HEADER_

#ifndef MMC_SUCCESS
	#define MMC_SUCCESS 0
#endif

#ifndef MMC_FAILED
	#define MMC_FAILED 1
#endif

#ifndef MMC_MAX_LOG_MSG_SIZE
	#define MMC_MAX_LOG_MSG_SIZE 512
#endif


class RoverMotor
{
public:
	/// for singleton implementation 
	static RoverMotor* Instance();

	// EPOS
	int openEpos();
	int disableEpos();
	int closeEpos();

	int home();
	int rotate(long speed_motor_a, long speed_motor_b);
	int readCurrent(short *);
	int stop();

private:
	/// for singleton implementation 
	static RoverMotor* pinstance; 
	/// for singleton implementation 
	RoverMotor(); 
	/// for singleton implementation 
	RoverMotor(const RoverMotor&); 
	/// for singleton implementation 
	RoverMotor& operator= (const RoverMotor&); 
	~RoverMotor(); 
};

#endif
