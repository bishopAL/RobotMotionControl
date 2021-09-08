#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <iostream>
#include <unistd.h>
#include "thread.h"
#include "motor.h"
#include <iostream>
#include <math.h>
#include <CppLinuxSerial/SerialPort.hpp>
#include <motionControl.h>
#include <Eigen/Core>
#include <js.h>
#define _JOYSTICK 1
#define THREAD1_ENABLE 1
#define THREAD2_ENABLE 1
#define THREAD3_ENABLE 1
#define THREAD4_ENABLE 1

using namespace std;

struct timeval startTime, endTime;
float loopRate1 = 10; //receive velocity data 10 Hz
float loopRate2 = 100; // send velocity & IMU data 10 Hz
float loopRate3 = 100; // update robot state 100 Hz
float loopRate4 = 100; // motion control 100 Hz

const int num = 12;
int ID[num] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

vector<float> present_position;
vector<float> present_velocity;
vector<int> present_torque;
float target_com_velocity[3];
float present_com_velocity[3];

float timePeriod = 0.01;
float timeForGaitPeriod = 0.49;
Matrix<float, 4, 2> timeForStancePhase;
MotionControl mc(timePeriod, timeForGaitPeriod, timeForStancePhase); // time

void *thread1_func(void *data) // receive velocity data
{
    struct timeval startTime1, endTime1;

    #ifdef _JOYSTICK
    int xbox_fd ;
    xbox_map_t map;
    int len, type;
    int axis_value, button_value;
    int number_of_axis, number_of_buttons ;
    float vel = 0;
    float theta = 0;
    memset(&map, 0, sizeof(xbox_map_t));
    xbox_fd = xbox_open("/dev/input/js0");
    map.lt = -32767;
    map.rt = -32767;
    while(1)
    {
        gettimeofday(&startTime1,NULL);
        len = xbox_map_read(xbox_fd, &map);
        if (len < 0)
        {
            usleep(10*1000);
            continue;
        }
        if (map.lx==0) theta = 0;
        else theta = float(atan2(-map.ly, map.lx) - 3.1416/2) / 5.0;
        vel = float(map.rt - map.lt) / 200.0;
        Vector<float, 3> tCV;
        tCV<<vel, 0.0, theta;
        mc.setCoMVel(tCV);
        cout<<tCV.transpose()<<", "<<-map.ly<<", "<<map.lx<<", "<<map.lt<<", "<<map.rt<<endl;
        gettimeofday(&endTime1,NULL);
        double timeUse = 1000000*(endTime1.tv_sec - startTime1.tv_sec) + endTime1.tv_usec - startTime1.tv_usec;
        fflush(stdout);
        // cout<<"thread1: "<<timeUse<<endl;
        // usleep(1/loopRate1*1e6 - (double)(timeUse) - 10); // /* 1e4 / 1e6 = 0.01s */
    }
    #endif
    // SerialPort serialPort("/dev/ttyUSB0", BaudRate::B_57600);
	// // Use SerialPort serialPort("/dev/ttyACM0", 13000); instead if you want to provide a custom baud rate
	// serialPort.SetTimeout(100); // Block when reading until any data is received
	// serialPort.Open();
    // string endFlag = "E";
}

void *thread2_func(void *data) // send velocity & IMU data
{
    struct timeval startTime2, endTime2;
    while(1)
    {
        gettimeofday(&startTime2,NULL);
        /*
        YOUR CODE HERE
        */
        gettimeofday(&endTime2,NULL);
        double timeUse = 1000000*(endTime2.tv_sec - startTime2.tv_sec) + endTime2.tv_usec - startTime2.tv_usec;
        // cout<<"thread2: "<<timeUse<<endl;
        usleep(1/loopRate2*1e6 - (double)(timeUse) - 10); // /* 1e4 / 1e6 = 0.01s */
    }
}


void *thread3_func(void *data) // update robot state
{// Port init start
    set_port_baudrate_ID("/dev/ttyAMA0", 3000000, ID, num);
    dxl_init();
    set_operation_mode(3); //3 position control; 0 current control
    torque_enable();
    usleep(1e6);
    struct timeval startTime3, endTime3;
    while(1)
    {
        gettimeofday(&startTime3,NULL);
        /*
        YOUR CODE HERE
        */
        // set_position(target_position);
        // get_position(present_position);
		// get_velocity(present_velocity);
		// get_torque(present_torque);
        get_position(present_position);
        get_velocity(present_velocity);
        for(uint8_t joints=0; joints<12; joints++)
        {
            mc.jointPresentPos(joints) = present_position[joints];
            mc.jointPresentVel(joints) = present_velocity[joints];
        }
        set_position(mc.jointCmdPos);
        gettimeofday(&endTime3,NULL);
        double timeUse = 1000000*(endTime3.tv_sec - startTime3.tv_sec) + endTime3.tv_usec - startTime3.tv_usec;  // us
        // cout<<"thread3: "<<timeUse<<endl;
        usleep(1/loopRate3*1e6 - (double)(timeUse) - 10); // Time for one period: 1/loopRate3*1e6 (us)
    }
}

void *thread4_func(void *data) // motion control, update goal position
{
    struct timeval startTime4, endTime4;
    Matrix<float, 4, 3> initPos; 
	initPos<< 3.0, 0.0, -225.83, 3.0, 0.0, -225.83, -20.0, 0.0, -243.83, -20.0, 0.0, -243.83;
	Vector<float, 3> tCV;
	tCV<<0.0, 0.0, 0.0;
	mc.setInitPos(initPos);
    cout<<(mc.timeForGaitPeriod - (mc.timeForStancePhase(0,1) - mc.timeForStancePhase(0,0)))/2<<endl;
    cout<<"initPos vel"<<endl;
	mc.setCoMVel(tCV);
    mc.inverseKinematics();
    usleep(3e6);
    while(1)
    {
        gettimeofday(&startTime4,NULL);
        /*
        YOUR CODE HERE
        */
        mc.nextStep();
        mc.inverseKinematics();
        gettimeofday(&endTime4,NULL);
        double timeUse = 1000000*(endTime4.tv_sec - startTime4.tv_sec) + endTime4.tv_usec - startTime4.tv_usec;
        // cout<<"thread4: "<<timeUse<<endl;
        usleep(1/loopRate4*1e6 - (double)(timeUse) - 10); // /* 1e4 / 1e6 = 0.01s */
    }
}


void thread_init()
{
    struct sched_param param;
    pthread_attr_t attr;
    pthread_t thread1 ,thread2 ,thread3, thread4;
    int ret;

    timeForStancePhase<<0, 0.24, 0.25, 0.49, 0.25, 0.49, 0, 0.24;
    mc.timeForStancePhase = timeForStancePhase;
    
    /* 1.Lock memory */
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
        printf("mlockall failed: %m\n");
        exit(-2);
    }
    /* 2. Initialize pthread attributes (default values) */
    ret = pthread_attr_init(&attr);
    if (ret) {
        printf("init pthread attributes failed\n");
        goto out;
    }
 
    /* 3. Set a specific stack size  */
    ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
    if (ret) {
        printf("pthread setstacksize failed\n");
        goto out;
    }
 
    /*4. Set scheduler policy and priority of pthread */
    ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if (ret) {
        printf("pthread setschedpolicy failed\n");
        goto out;
    }
    param.sched_priority = 20;
    ret = pthread_attr_setschedparam(&attr, &param);
    if (ret) {
            printf("pthread setschedparam failed\n");
            goto out;
    }
    /*5. Use scheduling parameters of attr */
        ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        if (ret) {
                printf("pthread setinheritsched failed\n");
                goto out;
        }
 
    /*6. Create a pthread with specified attributes */
        #ifdef THREAD1_ENABLE
        ret = pthread_create(&thread1, &attr, thread1_func, NULL);
        if (ret) {
                printf("create pthread1 failed\n");
                goto out;
        }
        
        #endif

        #ifdef THREAD2_ENABLE
        ret = pthread_create(&thread2, &attr, thread2_func, NULL);
        if (ret) {
                printf("create pthread2 failed\n");
                goto out;
        }
        #endif

        #ifdef THREAD3_ENABLE
        ret = pthread_create(&thread3, &attr, thread3_func, NULL);
        if (ret) {
                printf("create pthread3 failed\n");
                goto out;
        }
        #endif

        #ifdef THREAD4_ENABLE
        ret = pthread_create(&thread4, &attr, thread4_func, NULL);
        if (ret) {
                printf("create pthread4 failed\n");
                goto out;
        }
        #endif

        #ifdef THREAD1_ENABLE
        ret = pthread_join(thread1, NULL);
        if (ret)
            printf("join pthread1 failed: %m\n");
        #endif

        #ifdef THREAD2_ENABLE
        ret = pthread_join(thread2, NULL);
        if (ret)
            printf("join pthread2 failed: %m\n");
        #endif

        #ifdef THREAD3_ENABLE
        ret = pthread_join(thread3, NULL);
        if (ret)
            printf("join pthread3 failed: %m\n");
        #endif

        #ifdef THREAD4_ENABLE
        ret = pthread_join(thread4, NULL);
        if (ret)
            printf("join pthread4 failed: %m\n");
        #endif
 
    /*7. Join the thread and wait until it is done */
        

        

        

        
out:
    ret;

}

vector<string> split(const string& str, const string& delim) {  
	vector<string> res;  
	if("" == str) return res;  
	char * strs = new char[str.length() + 1] ; 
	strcpy(strs, str.c_str());   
 
	char * d = new char[delim.length() + 1];  
	strcpy(d, delim.c_str());  
 
	char *p = strtok(strs, d);  
	while(p) {  
		string s = p;   
		res.push_back(s); 
		p = strtok(NULL, d);  
	}  
 
	return res;  
}