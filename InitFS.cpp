#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>
#include "MyFileSystem.h"

using namespace std;

void initializeDataArea();
void initializeInodeArea();
void initializeSuperBlock();
void easycreateDinode(int num, int addr, int mode, int size, int nlink);

Buf     m_Buf[NBUF];
FILE    *fp;
unsigned char Buffer[NBUF][BUFFER_SIZE];

time_t nowtime;

DiskInode       dInod[80];    //占据3~12盘块
DirectoryEntry  direc[16];

void Initialize()
{
	fp = fopen("MyDisk.img", "wb+");
	for (int i = 0; i < NBUF; i++)
	{
		m_Buf[i].b_addr = Buffer[i];
	}
	initializeSuperBlock();  //spb占据1~2盘块
	initializeInodeArea();    //diskinode占据3~12盘块
	initializeDataArea();   //13~22盘块为DirectoryEntry区，文件数据从23盘块开始 到128盘块结束 共106块
	fclose(fp);
}
void initializeDataArea()
{
	int i,j, k,b,c,a;
	int free[100];
	DirectoryEntry dir[16];
	strcpy(direc[0].m_name, "/");
	direc[0].m_ino = 0;
	strcpy(direc[1].m_name, "bin");
	direc[1].m_ino = 1;
	strcpy(direc[2].m_name, "etc");
	direc[2].m_ino = 2;
	strcpy(direc[3].m_name, "home");
	direc[3].m_ino = 3;
	strcpy(direc[4].m_name, "usr");
	direc[4].m_ino = 4;
	strcpy(direc[5].m_name, "dev");
	direc[5].m_ino = 5;
	j = 13;
	k = Getbuf(j);
	for (a = 0; a < 512; a++)
	{
		Buffer[k][a] = 0;
	}
	char *p=(char*)&direc[0];
	for (b = 0; b < 16; b++)
	{
		p = (char*)&direc[b];
		Copy(p, (char*)m_Buf[k].b_addr + 32 * b, 32);
	}
	m_Buf[k].b_flags = Buf::B_WRITE;
	Strategy(&m_Buf[k], j);
	dir[0].m_ino = 0;
	strcpy(dir[0].m_name, "/");
	for (c = 0; c < 16; c++)
	{
		Copy((char*)&dir[c], (char*)m_Buf[k].b_addr + 32 * c, 32);
	}
	for (a = 14; a < 19; a++)
	{
		m_Buf[k].b_flags = Buf::B_WRITE;
		Strategy(&m_Buf[k], a);
	}
	for (b=2; b < 4; b++)
	{
		for (a = 0; a < 512; a++)
		{
			Buffer[0][a] = 0;
		}
		if (b != 3)
		{
			free[0] = b * 100 + 18;
		}
		else
		{
			free[0] = 262;
		}
		for (i = 1; i < 100; i++)
		{
			if (b == 3 && i >= 44)
			{
				free[i] = 0;
			}
			else
			{
				free[i] = (b - 1) * 100 + 18 + i;
			}
		}
		for (a = 0; a < 100; a++)
		{
			Copy((char*)&free[a], (char*)&Buffer[0][a * 4], sizeof(int));
		}
		m_Buf[0].b_flags = Buf::B_WRITE;
		Strategy(&m_Buf[0], free[0]-100);
	}
}
void initializeInodeArea()
{
	int j, k;
	easycreateDinode(0, 13, Inode::IFDIR, 512, 0);
	easycreateDinode(1, 14, Inode::IFDIR, 512, 1);
	easycreateDinode(2, 15, Inode::IFDIR, 512, 1);
	easycreateDinode(3, 16, Inode::IFDIR, 512, 1);
	easycreateDinode(4, 17, Inode::IFDIR, 512, 1);
	easycreateDinode(5, 18, Inode::IFDIR, 512, 1);
	for (int i = 0; i < 80; i++)
	{
		char* p = (char *)&dInod[i];
		j = (i / 8) + 3;
		k = Getbuf(j);
		m_Buf[k].b_flags = Buf::B_READ;
		Strategy(&m_Buf[k], j);
		Copy(p, (char *)m_Buf[k].b_addr+(i%8)*64, 64);
		m_Buf[k].b_flags = Buf::B_WRITE;
		Strategy(&m_Buf[k], j);         //要写的大小小于512字节，先读再写
	}
} //初始化的是diskinode
void initializeSuperBlock()
{
	SuperBlock sb;
	int i;
	sb.s_isize = 10;
	sb.s_fsize = 262;
	sb.s_nfree = 100;
	for (i = 1; i < 100; i++)
	{
		sb.s_free[i] = i + 18;
	}
	sb.s_free[0] = 118;
	sb.s_ninode = 74;
	for (i = 0; i < 74; i++)
	{
		sb.s_inode[i] = i+6;
	}
	for (; i < 100; i++)
	{
		sb.s_inode[i] = -1;
	}
	sb.s_flock = 0;
	sb.s_ilock = 0;
	sb.s_fmod = 0;
	sb.s_ronly = 0;
	nowtime = time(NULL);
	sb.s_time = (int)nowtime;
	for (i = 0; i < 47; i++)
	sb.padding[i] = 0;
	for (int i = 0; i < 2; i++)
	{
		char* p = (char *)&sb + i * 512;
		Copy(p, (char *)m_Buf[i].b_addr, 512);
		m_Buf[i].b_flags = Buf::B_WRITE;   //用位等于
		Strategy(&m_Buf[i], i + 1);
	}
}
void easycreateDinode(int num, int addr, int mode, int size, int nlink)
{
	dInod[num].d_mode = mode;
	dInod[num].d_nlink = nlink;
	dInod[num].d_addr[0] = addr;
	dInod[num].d_size = 0;
	nowtime = time(NULL);
	dInod[num].d_atime = (int)nowtime;
	nowtime = time(NULL);
	dInod[num].d_mtime = (int)nowtime;
}
