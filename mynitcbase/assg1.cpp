#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include<iostream>
using namespace std;
int main()
{
    Disk disk_run;
    unsigned char buffer[BLOCK_SIZE];
    Disk::readBlock(buffer,0);
    for(int i=0;i<10;i++)
    cout<<(int)buffer[i]<<" ";

    return 0;
}