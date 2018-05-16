#include <iostream>
#include <fstream>
#include <thread>
#include <string>
#include <mutex>

#include <geometry_msgs/WrenchStamped.h>

#include "Definitions.h"
#include "RoverMotor.h"
#include "TimerLinux.h"
#include <robot_comm/robot_comm.h>

using namespace std;

const string ROBOT_NAME = "abb120";

// parameters
const double robot_pose_0[7] = {0, 300, 400, 0, 0, 1, 0};
const double robot_pose_goal[7] = {100, 300, 400, 0, 0, 1, 0};
const double robot_speed = 50; // mm/s
const double motor_speed[2] = {100, 100};
const double duration_ms = 2000;
const string filepath = "~/Git/catkin_ws/src/RoverTest/data/";
const string folderpath = "~/Git/catkin_ws/src/RoverTest/data/pcd/";

// global variables
Timer timer;
ofstream maxon_file, ATI_file;
mutex netft_mtx;
double wrench[6];

// 
void* maxon_data_collection(void* pParam);
void* ATI_data_collection(void* pParam);
void* realsense_data_collection(void* pParam);
void ReceiveNetft_Callback(const geometry_msgs::WrenchStamped& w_msg);


int main(int argc, char** argv)
{

	/*************************************************
		Initialization
	*************************************************/

	// Initialize ROS
	ros::init(argc, argv, "Rover_experiments");
	ros::NodeHandle nh;
  	ros::Subscriber netft_sub = nh.subscribe("/netft/data", 1, &ReceiveNetft_Callback);


	// Initialize robot
	RobotComm robot(&nh, ROBOT_NAME);
	robot.SetWorkObject(0,0,0,1,0,0,0);
	robot.SetTool(0,0,0,1,0,0,0);
	robot.SetZone(0);
	robot.SetSpeed(robot_speed,50);
	robot.SetCartesian(robot_pose_0);
	cout << "[Main.RobotComm] Robot is initialized." << endl;	

	// initialize Maxon
	RoverMotor *rover = RoverMotor::Instance();
	rover->openEpos();

	cout << "[Main.Maxon] Press ENTER to begin homing.." << endl;	
	getchar();
	if(rover->home() != MMC_SUCCESS)
	{
		rover->closeEpos();
		return -1;
	} 

	
	/*************************************************
		Run
	*************************************************/

	cout << endl << "[Main] Initialization is done. Press ENTER to begin experiment.." << endl;	
	getchar();

	// file
    maxon_file.open(filepath + "maxon_data.txt");
    ATI_file.open(filepath + "ATI_data.txt");

	// timer
	timer.tic();
	// establish sensor threads
	pthread_t maxon_thread;
	pthread_t ATI_thread;
	pthread_t realsense_thread;
    int rc;
    rc = pthread_create(&maxon_thread, NULL, maxon_data_collection, &rover);
    if (rc){
        cout <<"[main] error:unable to create thread for maxon.\n";
        return false;
    }
    cout << "maxon thread created.\n";

    rc = pthread_create(&ATI_thread, NULL, ATI_data_collection, NULL);
    if (rc){
        cout <<"[main] error:unable to create thread for ATi.\n";
        return false;
    }
    cout << "ATI thread created.\n";

    rc = pthread_create(&realsense_thread, NULL, realsense_data_collection, NULL);
    if (rc){
        cout <<"[main] error:unable to create thread for realsense.\n";
        return false;
    }
    cout << "realsense thread created.\n";

	// Start maxon motors (non-blocking)
	if(rover->rotate(motor_speed[0], motor_speed[1]) != MMC_SUCCESS)
	{
		rover->closeEpos();
		return -1;
	} 
	// robot (blocking)
	robot.SetCartesian(robot_pose_goal);

	/*************************************************
		Exit
	*************************************************/

	cout << "[Main] Press ENTER to exit." << endl;	
	getchar();

	rover->stop();
	rover->closeEpos();
	maxon_file.close();
	ATI_file.close();
	
	return 0;
}

void* maxon_data_collection(void* pParam)
{
	RoverMotor *rover = (RoverMotor*)pParam;
	short current[2] = {0};
	double time = 0;
	while(true)
	{
		rover->readCurrent(current);
		time = timer.toc();
		if (time > duration_ms) break;
		maxon_file << time << "\t" << current[0] << "\t" << current[1] << endl;
	}
}
void* ATI_data_collection(void* pParam)
{
	double time = 0;
	while(true)
	{
		time = timer.toc();
		if (time > duration_ms) break;
		ATI_file << time << "\t" ;

		netft_mtx.lock();
		for (int i = 0; i < 6; ++i)
		{
			ATI_file << wrench[i] << "\t";
		}
		netft_mtx.unlock();

		ATI_file << endl;
	}
}

void* realsense_data_collection(void* pParam)
{

}


// subscribers
void ReceiveNetft_Callback(const geometry_msgs::WrenchStamped& w_msg)
{
  // Extract point cloud from ROS msg
  netft_mtx.lock();
  
  wrench[0] = w_msg.wrench.force.x;
  wrench[1] = w_msg.wrench.force.y;
  wrench[2] = w_msg.wrench.force.z;

  wrench[3] = w_msg.wrench.torque.x;
  wrench[4] = w_msg.wrench.torque.y;
  wrench[5] = w_msg.wrench.torque.z;

  netft_mtx.unlock();
}
