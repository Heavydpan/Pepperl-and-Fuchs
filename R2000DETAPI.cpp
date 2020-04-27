//#include "string.h"
#include "R2000DETAPI.h"
//#include "ros/ros.h"
#include "std_msgs/Int8MultiArray.h"
#include "std_msgs/Int8.h"
//#include "math.h"

char *R2000DET::ZoneBuffer = nullptr;
bool *R2000DET::Zone_Status = nullptr;
double R2000DET::x1 = 0;
double R2000DET::y1 = 0;
int R2000DET::ZoneofGroup = -1;
int R2000DET::iVersion = -1;
R2000DET::R2000DET()
{
	
}


R2000DET::~R2000DET()
{
}
void R2000DET::R2000StatusCallback(const sensor_msgs::LaserScan::ConstPtr& msg)
{
	if (ZoneBuffer == nullptr || iVersion!=1)
		return;
	bool *Zone_Status = isInZone_ROS(msg);

	ros::NodeHandle nh;//节点Handle
	ros::Publisher r2000detdata_pub = nh.advertise<std_msgs::Int8MultiArray>("R2000Status", 1000);
	std_msgs::Int8MultiArray status;

	status.data.resize(ZoneofGroup);
	for (int i = 0; i<ZoneofGroup; i++)
	{
		if (Zone_Status[i])
			status.data[i] = 1;
		else
			status.data[i] = 0;
	}
	r2000detdata_pub.publish(status);

	delete[]Zone_Status;
	Zone_Status = nullptr;
}

bool R2000DET::InitAPI()
{
	char filename[] = "Config";

	FILE *f;
	f = fopen(filename, "r");
	if (f == NULL)
	{
		//		perror("open file error:");//只有上面的函数设置了error全局错误号，才可使用，会根据error输出对应的错误信息
		return false;
	}
	fseek(f, 0L, SEEK_END); /* 定位到文件末尾 */
	int64_t flen = ftell(f); /* 得到文件大小 */
							 //ByteofConfigBuffer = flen;
	try {
		buff = new char[flen + 1];

	}
	catch (const std::bad_alloc& e) {
		return false;
	}
	
	fseek(f, 0L, 0);
	int64_t len=fread(buff, sizeof(char), flen, f);
	if (len != flen)
		return false;
	fclose(f);
	GetGroupNum();
	b_Init = true;
	return true;
}

bool R2000DET::GetGroupNum()
{
	if (buff == nullptr)
		return false;
	AllGroupNum = *(int*)buff;
	return true;
}

bool R2000DET::StartGroup(int i_Group)
{
	if (iVersion == -1)
	{
		if (buff == nullptr)//未初始化
			return false;
		ZoneBuffer = buff + GetConfigFilePos(i_Group, 1, buff) - sizeof(int);//含区域数
		ZoneofGroup = *(int*)ZoneBuffer;
		iVersion = 0;//普通版本标志
		return true;
	}
	return false;
	//因为每次仅启动一个分组，以下代码作废
	/*	char *tmp = nullptr;
	char *tmp1 = nullptr;//用于复制ZoneBuffer
	int64_t start = GetConfigFilePos(i_Group, 1, buff) - sizeof(int);//含区域数
	int64_t end = 0;
	if ((i_Group) <= AllGroupNum)
	{
	end = GetConfigFilePos(i_Group + 1, 1, buff) - sizeof(int);
	}
	int64_t size = end - start;//相当于tmp的长度
	tmp = (buff + GetConfigFilePos(i_Group, 1, buff) - sizeof(int));//含区域数的buff
	tmp1 = new char[size + ByteofZoneBuffer];
	memcpy(tmp1, ZoneBuffer, ByteofZoneBuffer);
	memcpy(tmp1 + ByteofZoneBuffer, tmp, size);
	GroupNumInZoneBuffer++;
	memcpy(tmp1, &GroupNumInZoneBuffer, sizeof(int));
	if (ZoneBuffer != nullptr)
	delete[]ZoneBuffer;
	ZoneBuffer = tmp1;
	ByteofZoneBuffer += size;*/


}

bool R2000DET::StartGroup_ROS(int i_Group, char Topic[])
{
	if (iVersion == -1)
	{
		if (buff == nullptr)//未初始化
			return false;
		ZoneBuffer = buff + GetConfigFilePos(i_Group, 1, buff) - sizeof(int);//含区域数
		ZoneofGroup = *(int*)ZoneBuffer;
		iVersion = 1;//普通版本标志
		char **tmp2 = nullptr;
		int tmp1 = 0;

		ros::init(tmp1, tmp2, "R2000DETData");
		ros::NodeHandle n;
		ros::Subscriber sub = n.subscribe(Topic, 1000, R2000DET::R2000StatusCallback);
		ros::spin();

		return true;
	}
	return false;
}

int64_t R2000DET::GetConfigFilePos(int i_Group, int i_Zone, char *buff)
{
	int64_t filelen1, tmpp = 0;

	int tmpq1 = 0, tmpz1 = 0;//tmpq一个用于读取当前组下区域数

	char * tmp1;
	tmp1 = buff;//指针移动，所有区域和组的数据
	tmpz1 = (*(int*)tmp1);
	tmp1 += sizeof(int);
	tmpq1 = (*(int*)tmp1);
	tmp1 += sizeof(int);
	filelen1 = sizeof(int) * 2;//定位
							   //bool flag = false;//用于区域数减1的标志；
	int i = 0, j = 0;

	for (i = 0; i < tmpz1; i++)
	{
		//找到需要删除的组和区域编号，进行定位
		for (j = 0; j < tmpq1; j++)
		{
			if (i == (i_Group - 1) && j == (i_Zone - 1))//i_Zone减2是因为要停在点数的开始
				break;
			tmpp = *(int64_t*)tmp1;
			filelen1 += tmpp * sizeof(CONFIG_XY) + sizeof(int64_t);
			tmp1 += tmpp * sizeof(CONFIG_XY) + sizeof(int64_t);
		}
		if (i == (i_Group - 1) && j == (i_Zone - 1))//减2是因为要停在点数的开始
		{
			//					filelen1 += sizeof(int);
			break;
		}
		tmpq1 = *(int*)tmp1;
		tmp1 += sizeof(int);
		filelen1 += sizeof(int);
	}

	return filelen1;
}
bool R2000DET::StopGroup(int i_Group)
{
	//	ByteofZoneBuffer = sizeof(int);
	if (ZoneBuffer != nullptr)
	{
		delete[]ZoneBuffer;
		ZoneBuffer = nullptr;
		Zone_Status = nullptr;
		if (iVersion == 1)
		{

		}
		ZoneofGroup = -1;
	}
	iVersion = -1;
	return true;
}
bool R2000DET::SetCW_CCW(bool CW_CCW)
{
	if (buff == nullptr)
		return false;
	CW_CCW = CW_CCW;
	return true;
}
bool* R2000DET::isInZone_ROS(const sensor_msgs::LaserScan::ConstPtr& PointBuffer)
{
	if (PointBuffer == nullptr || ZoneBuffer == nullptr)
		return nullptr;
	char *Buffer = ZoneBuffer;


	int *qs = (int*)Buffer;
	try {
		Zone_Status = new bool[*qs];

	}
	catch (const std::bad_alloc& ba) {
		return nullptr;
	}
	
	memset(Zone_Status, 0, sizeof(bool)*(*qs));

	uint64_t ds = 0;
	Buffer += sizeof(int);
	int i = 0, count = 0;//count为连续计数;
	uint64_t tmpi = 0;
	float tmpx = -1;
	float tmpy = -1;

	double angular_increment_real = PointBuffer->angle_increment;//360 / (double)R2000Head.num_points_scan;弧度

	double distance;//uint32_t，按照博智林要求改
	double ang;

	for (uint32_t i1 = 0; i1 <PointBuffer->ranges.size(); i1++)
	{

		distance =(double) PointBuffer->ranges[i1]*1000;//博智林，单位米，换算成毫米
//		if (distance == 0xFFFFFFFF) //按照博智林要求删除
//			break;

		ang = PointBuffer->angle_min + angular_increment_real*i1;
//		y1 = distance*cos(ang) / 62.5;
//		x1 = distance*sin(ang) / 62.5;
		y1 = -1*distance*sin(ang) / 62.5;
		x1 = distance*cos(ang) / 62.5;
		//坐标平移，以(480,480)为圆心。注意原始config坐标数据是屏幕坐标，需要转换
		x1 += 480;
		y1 += 480;

		for (i = 0; i < *qs; i++)
		{
			ds = *(uint64_t*)(Buffer); //+ds[0]*j*sizeof(CONFIG_XY));
			CONFIG_XY* xy = nullptr;//new CONFIG_XY[ds[0]+1];
			xy = (CONFIG_XY*)(Buffer + sizeof(int64_t));// *(j + 1) + sizeof(CONFIG_XY)*ds[0] * j);
			for (uint64_t j = 0; j < ds; j++)
			{
				if (fabs((float)y1 - xy[j].y) <= 0.1)//找Y轴
				{
					tmpy = xy[j].y;
					tmpx = xy[j].x;
					if (isInOneZone(tmpx, tmpy, x1, Buffer))//相当于判断X轴
					{

						if (tmpi == 0 && i1 == 0)
							count++;
						else
						{
							if (tmpi >= 0 && (i1 - tmpi) == 1)//判断是否连续
							{
								count++;
								tmpi = i1;
							}
							else
							{
								tmpi = i1;
								count = 1;
							}
						}
						if (count == 2)//连续2个点
						{
							Zone_Status[i] = true;
							count = 0;
						}
						break;
					}
				}


			}
			Buffer += sizeof(int64_t) + ds * sizeof(CONFIG_XY);
		}
		Buffer = ZoneBuffer + sizeof(int);
	}
	return Zone_Status;

}
bool* R2000DET::isInZone(char* PointBuffer)
{
	if (PointBuffer == nullptr || ZoneBuffer == nullptr)
		return nullptr;
	char *Buffer = ZoneBuffer;
	bool *Zone_Status = nullptr;//每个区域状态
								//	Buffer += sizeof(int);//因为包含分组数量
	int *qs = (int*)Buffer;
	try {
		Zone_Status = new bool[*qs];

	}
	catch (const std::bad_alloc& e) {
		return nullptr;
	}
//	Zone_Status = new bool[*qs];
	memset(Zone_Status, 0, sizeof(bool)*(*qs));

	uint64_t ds = 0;
	Buffer += sizeof(int);
	int i = 0, count = 0;//count为连续计数;
	uint64_t tmpi = 0;
	float tmpx = -1;
	float tmpy = -1;
	R2000Head = *(tmpData*)PointBuffer;
	double angular_increment_real = 360 / (double)R2000Head.num_points_scan;
	PointBuffer += sizeof(R2000Head);
	uint32_t distance;
	double ang;
	uint32_t times = (uint32_t)((R2000Head.packet_size - sizeof(R2000Head)) / 6);
	for (uint32_t i1 = 0; i1 <times; i1++)
	{

		distance = *(uint32_t*)(PointBuffer + i1 * 6);
		if (distance == 0xFFFFFFFF)
			break;
		if (CW_CCW)
			ang = R2000Head.first_angle / 10000 + angular_increment_real*i1;
		else
			ang = R2000Head.first_angle / 10000 - angular_increment_real*i1;
		a2b(ang, distance / 62.5);
		//坐标平移，以(480,480)为圆心。注意原始config坐标数据是屏幕坐标，需要转换
		x1 += 480;
		y1 += 480;

		for (i = 0; i < *qs; i++)
		{
			ds = *(uint64_t*)(Buffer); //+ds[0]*j*sizeof(CONFIG_XY));
			CONFIG_XY* xy = nullptr;//new CONFIG_XY[ds[0]+1];
			xy = (CONFIG_XY*)(Buffer + sizeof(int64_t));// *(j + 1) + sizeof(CONFIG_XY)*ds[0] * j);
			for (uint64_t j = 0; j < ds; j++)
			{
				if (fabs((float)y1 - xy[j].y) <= 0.1)//找Y轴
				{
					tmpy = xy[j].y;
					tmpx = xy[j].x;
					if (isInOneZone(tmpx, tmpy, x1, Buffer))//相当于判断X轴
					{

						if (tmpi == 0 && i1 == 0)
							count++;
						else
						{
							if (tmpi >= 0 && (i1 - tmpi) == 1)//判断是否连续
							{
								count++;
								tmpi = i1;
							}
							else
							{
								tmpi = i1;
								count = 1;
							}
						}
						if (count == 2)//连续2个点
						{
							Zone_Status[i] = true;
							count = 0;
						}
						break;
					}
				}


			}
			Buffer += sizeof(int64_t) + ds * sizeof(CONFIG_XY);
		}
		Buffer = ZoneBuffer + sizeof(int);
	}
	return Zone_Status;


}
bool R2000DET::isInOneZone(float tmpx, float tmpy, double x, char *Buffer)
{
	bool flag = false;
	uint64_t ds;
	//ds = *(uint64_t*)(ZoneBuffer);
	ds = *(uint64_t*)Buffer;
	CONFIG_XY* xy = nullptr;//new CONFIG_XY[ds[0]+1];
	xy = (CONFIG_XY*)(Buffer + sizeof(uint64_t));// *(j + 1) + sizeof(CONFIG_XY)*ds[0] * j);
	for (uint64_t j = 0; j < ds; j++)
	{
		if ((tmpy >= 0) && (fabs((float)tmpy - xy[j].y) <= 0.1))
		{

			if (x <= (tmpx >= xy[j].x ? tmpx : xy[j].x) && (x >= (tmpx <= xy[j].x ? tmpx : xy[j].x)))
			{
				flag = true;
				break;
			}

		}

	}
	return flag;
}

bool* R2000DET::GetStatus(char* PointBuffer)
{
	/*	if (Zone_Status != nullptr)
	{
	delete[]Zone_Status;
	Zone_Status = nullptr;
	}*/
	if (iVersion == 0)
	{
		bool* Status = isInZone(PointBuffer);
		return Status;
	}
	return nullptr;
}
void R2000DET::a2b(double angle, double distance)
{
	//	x1 = distance*cos(angle / 180 * 3.1415926535);
	//	y1 = distance*sin(angle / 180 * 3.1415926535);
	y1 = distance*cos(angle / 180 * 3.1415926535);
	x1 = distance*sin(angle / 180 * 3.1415926535);
}
bool R2000DET::EndAPI()
{
	b_Init = false;
	if (Zone_Status != nullptr)
		delete[]Zone_Status;
	if (ZoneBuffer != nullptr)
		delete[]ZoneBuffer;
	if (buff != nullptr)
		delete[]buff;
	CW_CCW = true;
	iVersion = -1;
	return true;
}