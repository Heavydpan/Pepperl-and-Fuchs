#include "R2000DET.h"



R2000DET::R2000DET()
{
}


R2000DET::~R2000DET()
{
}

bool R2000DET::InitAPI()
{
	char filename[] = "Config";
	
	FILE *f;
	f = fopen(filename, "r");
	if (f == NULL)
	{
		perror("open file error:");//只有上面的函数设置了error全局错误号，才可使用，会根据error输出对应的错误信息
		return false;
	}
	fseek(f, 0L, SEEK_END); /* 定位到文件末尾 */
	int64_t flen = ftell(f); /* 得到文件大小 */
	ByteofConfigBuffer = flen;
	buff = new char[flen + 1];
	fseek(f, 0L, 0);
	fread(buff, sizeof(char), flen, f);
	fclose(f);
	GetGroupNum();
	b_Init = true;
	return true;
}

bool R2000DET::GetGroupNum()
{
	if(buff==nullptr)
		return false;
	AllGroupNum =*(int*)buff;
	return true;
}

bool R2000DET::StartGroup(int i_Group)
{
	if (buff == nullptr)//未初始化
		return false;
	char *tmp = nullptr;
	char *tmp1 = nullptr;//用于复制ZoneBuffer
	int64_t start = GetConfigFilePos(i_Group, 1, buff) - sizeof(int);//含区域数
	int64_t end = 0;
	if ((i_Group + 1) <= AllGroupNum)
	{
		end = GetConfigFilePos(i_Group + 1, 1, buff) - sizeof(int);
	}
	int64_t size = end - start;//相当于tmp的长度
	tmp = (buff + GetConfigFilePos(i_Group, 1, buff) - sizeof(int));
	
	tmp1 = new char[size + ByteofZoneBuffer];
	memcpy(tmp1, ZoneBuffer, ByteofZoneBuffer);
	memcpy(tmp1 + ByteofZoneBuffer, tmp, size);
	GroupNumInZoneBuffer++;
	memcpy(tmp1, &GroupNumInZoneBuffer, sizeof(int));
	if (ZoneBuffer != nullptr)
		delete[]ZoneBuffer;
	ZoneBuffer = tmp1;
	ByteofZoneBuffer += size;

	return true;
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
	bool flag = false;//用于区域数减1的标志；
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
	return false;
}
bool* R2000DET::isInZone(char* PointBuffer)
{
	if (PointBuffer == nullptr)
		return false;
	char *Buffer = ZoneBuffer;
	Buffer += sizeof(int);//因为包含分组数量
	int *qs = (int*)Buffer;
	Zone_Status=new bool [*qs];
	memset(Zone_Status, 0, sizeof(bool)*(*qs));

	int64_t *ds = nullptr;
	Buffer += sizeof(int);
	int i = 0;
	float tmpx = -1;
	float tmpy = -1;
//	bool flag = false;
	R2000Head = *(tmpData*)PointBuffer;
	double angular_increment_real = 360 / (double)R2000Head.num_points_scan;
	PointBuffer += sizeof(R2000Head);
	uint32_t *distance;
	double ang;
	for (uint32_t i = 0; i < (R2000Head.packet_size-60) / 6; i++)
	{

		distance = (uint32_t*)(PointBuffer + i * 6);
		if (*distance == -1)
			break;
		ang = angular_increment_real*i + R2000Head.first_angle / 10000;
		a2b(ang, *distance / 62.5);

		for (i = 0; i < *qs; i++)
		{
			ds = (int64_t*)(Buffer); //+ds[0]*j*sizeof(CONFIG_XY));
			CONFIG_XY* xy = nullptr;//new CONFIG_XY[ds[0]+1];
			xy = (CONFIG_XY*)(Buffer + sizeof(int64_t));// *(j + 1) + sizeof(CONFIG_XY)*ds[0] * j);
			for (int64_t j = 0; j < ds[0]; j++)
			{
				if (fabs((float)y1 - xy[j].y) <= 0.01)//找Y轴
				{
					tmpy = xy[j].y;
					tmpx = xy[j].x;
					if (isInOneZone(tmpx, tmpy, x1,y1))//相当于判断X轴
					{
					//	flag = true;
					//	goto check;
						Zone_Status[i] = true;
						break;
					}
				}

			}
			Buffer += sizeof(int64_t) + ds[0] * sizeof(CONFIG_XY);
		}
		
	}
	return Zone_Status;


	//	return -1;
/*check:

	if (!flag)
		return -1;
	return i + 1;*/

}
bool R2000DET::isInOneZone(float tmpx, float tmpy, int x, int y)
{
	bool flag = false;
	uint64_t *ds = nullptr;
	ds = (uint64_t*)(ZoneBuffer);
	CONFIG_XY* xy = nullptr;//new CONFIG_XY[ds[0]+1];
	xy = (CONFIG_XY*)(ZoneBuffer + sizeof(uint64_t));// *(j + 1) + sizeof(CONFIG_XY)*ds[0] * j);
	for (uint64_t j = 0; j < ds[0]; j++)
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
	if (Zone_Status != nullptr)
	{
		delete[]Zone_Status;
		Zone_Status = nullptr;
	}
	bool* Status = isInZone(PointBuffer);


	return Status;
}
void R2000DET::a2b(double angle, double distance)
{
	x1 = distance*cos(angle / 180 * 3.1415926535);
	y1 = distance*sin(angle / 180 * 3.1415926535);
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
	return true;
}
