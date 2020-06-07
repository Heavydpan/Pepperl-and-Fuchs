#include <iostream>
#include "stdio.h"
#include "ros/ros.h"
#include "sensor_msgs/LaserScan.h"
#include<new> 
//#include <exception>
#pragma once

typedef struct
{
	int i_TotalGroup;
}CONFIG_HEAD;
typedef struct
{
	//		int i_Group=15;//分组标志位
	int i_TotalZone;//对应分组的区域数
}CONFIG_GROUP_NUM;
typedef struct
{
	//		int i_Zone=16;//区域标志位
	int64_t l_NumofPoints;//对应区域点数
}CONFIG_ZONE_NUM;
typedef struct//对应区域编号的坐标数据
{
	float x;
	float y;
}CONFIG_XY;

class R2000DET
{
public:
	R2000DET();
	~R2000DET();

	bool b_Init = false;//是否初始化成功
	bool InitAPI();
	bool SetCW_CCW(bool CW_CCW);//true:CCW,false:CW
	bool StartGroup(int i_Group);
	bool StartGroup_ROS(int i_Group, char Topic[]);
	bool StopGroup(int i_Group);
	bool EndAPI();
	bool *GetStatus(char* PointBuffer);//返回避障判断信息
	int AllGroupNum = -1;//Config的所有组数
	static int ZoneofGroup;//当前组的区域数
	int Group = -1;//当前使用的分组编号
	struct tmpData
	{
		uint16_t magic;
		uint16_t packet_type;
		uint32_t packet_size;
		uint16_t header_size;
		uint16_t scan_number;
		uint16_t packet_number;
		uint64_t timestamp_raw;
		uint64_t timestamp_sync;
		//	UINT32 status_flags;
		uint16_t status_flags;
		uint16_t scan_frequency;
		uint16_t status_flags1;
		uint16_t num_points_scan;
		uint16_t num_points_packet;
		uint16_t first_index;
		int32_t first_angle;
		int32_t angular_increment;
		uint32_t output_status;
		uint32_t field_status;
	}R2000Head;
private:
	bool GetGroupNum();
	int64_t GetConfigFilePos(int i_Group, int i_Zone, char *buff);
	//int64_t ByteofConfigBuffer = 0;
	//int GroupNumInZoneBuffer = 0;//ZoneBuffer里面包含的分组数
	bool* isInZone(char* PointBuffer);//PointBuffer R2000原始数据
	static		bool* isInZone_ROS(const sensor_msgs::LaserScan::ConstPtr& PointBuffer);
	static bool isInOneZone(float tmpx, float tmpy, double x, uint32_t start, char *Buffer);
	static int iVersion;//0:普通版本，1:ROS，2、3……，默认：-1，未选择
	void a2b(double angle, double distance);
	static double x1;
	static double y1;
	char * buff = nullptr;//所有Config
	static	char * ZoneBuffer;//每次仅启用一个分组，所以是区域数+区域数据
	static bool *Zone_Status;//每个区域状态
	bool CW_CCW = true;
	static void R2000StatusCallback(const sensor_msgs::LaserScan::ConstPtr& msg);
 
};