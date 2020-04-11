#include "R2000DETAPI.h"
//#include "ros/ros.h"
#include "std_msgs/String.h"


int main(int argc, char **argv)
{
  R2000DET test;
  bool t=test.InitAPI();
  if(t)
    std::cout<<"INIT API SUCESS"<<std::endl;
  return 0;
}
