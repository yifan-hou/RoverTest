#ifndef _ZZJHAND_CLASS_HEADER_
#define _ZZJHAND_CLASS_HEADER_

#ifndef MMC_SUCCESS
	#define MMC_SUCCESS 0
#endif

#ifndef MMC_FAILED
	#define MMC_FAILED 1
#endif

#ifndef MMC_MAX_LOG_MSG_SIZE
	#define MMC_MAX_LOG_MSG_SIZE 512
#endif


// mechanical parameters
#define ENCODER_COUNT 8192
#define OPEN_FINGER_SIGN 1 // +1 or -1
#define MM2COUNT 7964


// control parameters

// current for closing finger
#define CURRENT_BREAKAWAY_CLOSING 800 // mA
#define CURRENT_PIVOT_GRASP 350
#define CURRENT_FIRM_GRASP 500

// current for getHomePos()
#define CURRENT_BREAKAWAY_OPENING 1000 // mA
#define CURRENT_OPEN_FINGER 400

// #define OPEN_FINGER_DIST 500000

class ZZJHand
{
public:
	/// for singleton implementation 
	static ZZJHand* Instance();

	// EPOS
	int openEpos();
	int disableEpos();
	int closeEpos();

	int openFinger();
	int getHomePos();
	int DoPivotGrasp();
	int DoFirmGrasp();

private:
	// low level
	int closeFinger(int);

	/// for singleton implementation 
	static ZZJHand* pinstance; 
	/// for singleton implementation 
	ZZJHand(); 
	/// for singleton implementation 
	ZZJHand(const ZZJHand&); 
	/// for singleton implementation 
	ZZJHand& operator= (const ZZJHand&); 
	~ZZJHand(); 

	int _home_pos;
	bool _home_pos_set;

};

#endif
