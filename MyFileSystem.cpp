#include "MyFileSystem.h"

#include <stdio.h>
#include <iostream>
#include <time.h>
#include <io.h>


using namespace std;

extern FILE *fp;   //全局文件指针

extern Buf m_Buf[NBUF];           //缓存控制块m_buf[15]
extern unsigned char Buffer[NBUF][BUFFER_SIZE];            //buffer[15][512]缓存

extern time_t nowtime;

SuperBlock  g_spb;      //superblock全局变量
Inode       inode[32];  //内存inode节点共32个

DiskInode   dskinode;   //diskinode全局变量

DirectoryEntry dir;     //全局目录项
DirectoryEntry cdir;    //当前打开目录项

/*==============================class Inode===================================*/
Inode::Inode()
{
	/* 将Inode对象的成员变量初始化为无效值 */
	this->i_flag = 0;
	this->i_mode = 0;
	this->i_count = 0;
	this->i_nlink = 0;
	this->i_dev = -1;
	this->i_number = -1;
	this->i_uid = -1;
	this->i_gid = -1;
	this->i_size = 0;
	this->i_lastr = -1;
	for(int i = 0; i < 10; i++)
	{
		this->i_addr[i] = 0;
	}
}

Inode::~Inode()
{
	//nothing to do here
}

/*============================class DiskInode=================================*/

DiskInode::DiskInode()
{
	/*
	 * 如果DiskInode没有构造函数，会发生如下较难察觉的错误：
	 * DiskInode作为局部变量占据函数Stack Frame中的内存空间，但是
	 * 这段空间没有被正确初始化，仍旧保留着先前栈内容，由于并不是
	 * DiskInode所有字段都会被更新，将DiskInode写回到磁盘上时，可能
	 * 将先前栈内容一同写回，导致写回结果出现莫名其妙的数据。
	 */
	this->d_mode = 0;
	this->d_nlink = 0;
	this->d_uid = -1;
	this->d_gid = -1;
	this->d_size = 0;
	for(int i = 0; i < 10; i++)
	{
		this->d_addr[i] = 0;
	}
	this->d_atime = 0;
	this->d_mtime = 0;
}

DiskInode::~DiskInode()
{
	//nothing to do here
}

/*============================class SuperBlock=================================*/

SuperBlock::SuperBlock()
{
	//nothing to do here
}
SuperBlock::~SuperBlock()
{
	//nothing to do here
}

/*============================class Buf=================================*/
Buf::Buf()
{
	this->b_flags = 0;
	this->b_blkno = -1;
	nowtime = time(NULL);
	this->b_time = (int)nowtime;
}

Buf::~Buf()
{

}

/*============================class DirectoryEntry=================================*/
DirectoryEntry::DirectoryEntry()
{
	this->m_ino = -1;
	for (int i = 0; i < DirectoryEntry::DIRSIZ; i++)
	{
		this->m_name[i] = '\0';
	}
}

DirectoryEntry::~DirectoryEntry()
{
	//nothing to do here
}


/*
 *
 *   FileSystem 函數的定義
 *
 */

void Copy(char *src, char *dst, int count)        //拷贝函数，从src内存拷贝count个字节到dst内存
{
	while (count--)
	{
		*dst = *src;
		dst++;
		src++;
	}
	return;
}

int Getaddr(int fd, int num)    //通过内存节点和其num号数据块得到该块地址
{
	int k;
	if (num <= 5)      //addr[0~5]
	{
		return inode[fd].i_addr[num];
	}
	else if (num > 5 && num <= 261)       //addr[6,7]一级索引块
	{
		if (inode[fd].i_addr[((num - 6) / 128) + 6] != -1)
		{
			k = Getbuf(inode[fd].i_addr[(num - 6) / 128 + 6]);
			m_Buf[k].b_flags = Buf::B_READ;
			Strategy(&m_Buf[k], inode[fd].i_addr[(num - 6) / 128 + 6]);

			int p = (int)Buffer[k][((num - 6) % 128) * sizeof(int)];
			if (p != 0)
			{
				return p;
			}
			else
			{
				return -1;
			}
		}
		else
		{
			return -1;
		}
	}
	else
	{
		cout << "请输入正确的命令！" << endl;
		return -2;
	}
}

void ls()              //列目录函数
{
	DirectoryEntry direc[16];
	int i,j,k,a;
	i = cdir.m_ino;
	j = icopy(i);
	if (inode[j].i_mode == Inode::IFDIR)
	{
		k = Getbuf(inode[j].i_addr[0]);
		m_Buf[k].b_flags = Buf::B_READ;
		Strategy(&m_Buf[k], inode[j].i_addr[0]);           //读入当前目录所占据的扇区

		for (a = 0; a < 16; a++)
		{
			Copy((char*)m_Buf[k].b_addr + a * 32, (char*)&direc[a], 32);   //读取16个directoryentry
		}
		for (a = 1; a < 16; a++)          //第一个directoryentry（父目录）不列出
		{
			if (direc[a].m_ino != -1)
			{
				cout << direc[a].m_name << "     ";
			}
		}
	}
	cout << endl;
}

int fclose(int fd)                     //关闭文件，被修改过的时候需要写回的内容已经在fwrite中实现
{
	if (inode[fd].i_number == -1)
	{
		cout << "无此文件！" << endl;
		return 0;
	}
	inode[fd].i_number = -1;
	cout << "关闭文件成功" << endl;
	return 1;
}

int icopy(int fd)   //将可能需要用到的i节点结构拷贝到内存中
{
	DiskInode dInode;
	int i, fd1, j;

	for (i = 0; i < 32; i++)
	{
		if (inode[i].i_number == fd)      //已经在内存中，直接返回编号
		{
			return i;
		}
	}
	for (i = 0; i < 32; i++)
		{
			if (inode[i].i_number == -1)     //找到空闲内存i节点
			{
				break;
			}
		}
		if (i == 32)
		{
			cout << "当前打开文件数量过多，请输入要关闭文件的内存i节点编号，关闭文件！" << endl;
			cin >> fd1;
			i = fclose(fd1);
			return -1;
		}
		j = (fd / 8) + 3;
		int k = Getbuf(j);

		m_Buf[k].b_flags = Buf::B_READ;   //从磁盘中diskinode读取关于该文件的各种信息
		Strategy(&m_Buf[k], j);

		char* p = (char *)&dInode;
		Copy((char *)m_Buf[k].b_addr + (fd % 8) * 64, p, 64);

		inode[i].i_number = fd;
		inode[i].i_mode = dInode.d_mode;
		inode[i].i_nlink = dInode.d_nlink;
		inode[i].i_uid = dInode.d_uid;
		inode[i].i_gid = dInode.d_gid;
		inode[i].i_size = dInode.d_size;
		memcpy(inode[i].i_addr, dInode.d_addr, 10 * sizeof(int));
		return i;
}

int flseek(int fd, int position)         //改变文件读写指针
{
	inode[fd].i_position=position;
	return 0;
}

int fcreate(char *name, int mode)   //在根目录下创建名为name的文件
{
	int blkno,di,j,k,b;
	DirectoryEntry dire[16];
	if (g_spb.s_nfree > 0)              //使用一个空闲盘块
	{
		g_spb.s_nfree--;
		blkno = g_spb.s_free[g_spb.s_nfree];
		g_spb.s_fmod = 1;
	}
	else if (g_spb.s_free[0] != 2018)             //nfree==0的时候的扩增s_free的算法
	{
		k = Getbuf(g_spb.s_free[0]);
		m_Buf[k].b_flags = Buf::B_READ;
		Strategy(&m_Buf[k], g_spb.s_free[0]);
		Copy((char*)m_Buf[k].b_addr, (char*)&g_spb.s_free[0], 100 * sizeof(int));
		g_spb.s_nfree = 100;
		g_spb.s_nfree--;
		blkno = g_spb.s_free[g_spb.s_nfree];
		g_spb.s_fmod = 1;
	}
	else
	{
		cout << "剩余空间不足，无法完成文件创建。" << endl;
		return -1;
	}
	if (mode != 1 && mode != 0)
	{
		cout << "请输入正确的文件类型后重试。" << endl;
		return -2;
	}
	if (g_spb.s_ninode >= 0)
	{
		di = g_spb.s_inode[--g_spb.s_ninode];
		g_spb.s_fmod = 1;
	}
	else
	{
		cout << "文件数量已满，请删除不必要的文件" << endl;
		return -3;
	}
	//int a = 0;
	j = (cdir.m_ino / 8) + 3;          //以下为读取当前的目录文件，写入name到目录文件中的算法
    k = Getbuf(j);
	m_Buf[k].b_flags = Buf::B_READ;
	Strategy(&m_Buf[k], j);
	Copy((char*)m_Buf[k].b_addr + (cdir.m_ino % 8) * 64, (char*)&dskinode,64);
	j = dskinode.d_addr[0];
	k = Getbuf(j);
	m_Buf[k].b_flags = Buf::B_READ;
	Strategy(&m_Buf[k], j);
	for (int i = 0; i < 16; i++)
	{
		Copy((char*)m_Buf[k].b_addr + 32 * i, (char*)&dire[i], 32);
	}
	for (b = 0; b < 16; b++)
	{
		if (dire[b].m_ino == -1)
		{
			break;
		}
	}
	if (b == 16)
	{
		cout << "该目录下的文件已满，请删除子文件或目录后重试" << endl;
		return -1;
	}
	dire[b].m_ino = di;
	strcpy(dire[b].m_name , name);
	Copy((char*)&dire[b], (char*)m_Buf[k].b_addr + 32 * b, 32);
	m_Buf[k].b_flags = Buf::B_WRITE;
	Strategy(&m_Buf[k], j);
	if (mode == 1)               //这里开始修改dskinode对象，并将其写入diskinode区域中的指定区域（di号diskinode）
	{
		dskinode.d_mode = Inode::IFDIR;
		DirectoryEntry d[16];
		k = Getbuf(blkno);
		m_Buf[k].b_flags = Buf::B_READ;
		Strategy(&m_Buf[k], blkno);
		d[0].m_ino = cdir.m_ino;
		strcpy(d[0].m_name , cdir.m_name);
		for (j = 0; j < 16; j++)
		{
			Copy((char*)&d[j], (char*)m_Buf[k].b_addr + 32 * j, 32);
		}
		m_Buf[k].b_flags = Buf::B_WRITE;
		Strategy(&m_Buf[k], blkno);
		dskinode.d_size = 512;
		nowtime = time(NULL);
		dskinode.d_atime = (int)nowtime;
		nowtime = time(NULL);
		dskinode.d_mtime = (int)nowtime;
	}
	else
	{
		dskinode.d_mode = 0;
		k = Getbuf(blkno);
		for (j = 0; j < 512; j++)
		{
			Buffer[k][j] = 0;
		}
		m_Buf[k].b_flags = Buf::B_WRITE;
		Strategy(&m_Buf[k], blkno);
		dskinode.d_size = 0;
		nowtime = time(NULL);
		dskinode.d_atime = (int)nowtime;
		nowtime = time(NULL);
		dskinode.d_mtime = (int)nowtime;
	}
	dskinode.d_addr[0]=blkno;
	j = (di / 8) + 3;
	k = Getbuf(j);
	m_Buf[k].b_flags = Buf::B_READ;
	Strategy(&m_Buf[k], j);
	char *p = (char*)&dskinode;
	Copy(p, (char*)m_Buf[k].b_addr + (di % 8) * 64, 64);
	m_Buf[k].b_flags = Buf::B_WRITE;
	Strategy(&m_Buf[k], j);
	cout << "文件创建成功" << endl;
	return di;
}

int fdelete(char *name)
{
	DirectoryEntry dire[16];
	int i, j, k, a,c;
	int blkno[100] = { 0 };                //blkno数组用于储存可以获得释放的扇区
	i = cdir.m_ino;
	j = icopy(i);
	c = inode[j].i_addr[0];
	k = Getbuf(c);
	m_Buf[k].b_flags = Buf::B_READ;
	Strategy(&m_Buf[k], c);
	for (a = 0; a < 16; a++)
	{
		Copy((char*)m_Buf[k].b_addr + a * 32, (char*)&dire[a], 32);
	}
	for (a = 0; a < 16; a++)
	if (strcmp(name, dire[a].m_name) == 0)             //找到该文件
	{
	    break;
	}
	if (a == 16)
	{
		cout << "未找到该文件" << endl;
		return 0;
	}
	dire[a].m_ino = -1;               //对应目录项置为初始值
	for (j = 0; j < DirectoryEntry::DIRSIZ; j++)
	{
		dire[a].m_name[j] = '\0';
	}
	Copy((char*)&dire[a], (char*)m_Buf[k].b_addr + 32 * a, 32);
	m_Buf[k].b_flags = Buf::B_WRITE;
	Strategy(&m_Buf[k], c);                //目录内容写回
	i = icopy(dire[a].m_ino);
	j = (dire[a].m_ino / 8) + 3;
	k = Getbuf(j);
	m_Buf[k].b_flags = Buf::B_READ;
	Strategy(&m_Buf[k], j);              //将磁盘上diskinode所在扇区拷贝至缓存
	char *p = (char *)&dskinode;          //将dskinode地址赋到p指针
	for (int b = 0;Getaddr(a,b) != -1; b++ )
	{
		blkno[b] = Getaddr(a, b);
	}
	for (int n = 0; n < 10; n++ )            //dskinode置空，准备写回硬盘要删除那个文件的diskinode部分
	{
		dskinode.d_addr[n] = -1;
	}
	dskinode.d_size = 0;
	dskinode.d_mode = 0;
	dskinode.d_atime = 0;
	dskinode.d_mtime = 0;
	Copy(p, (char*)m_Buf[k].b_addr+(i%8)*64, sizeof(DiskInode));
	m_Buf[k].b_flags = Buf::B_WRITE;
	Strategy(&m_Buf[k], j);                         //写diskinode回硬盘
	for (a = 0; a < 100&&blkno[a]!=0; a++)       //释放空闲盘块
	{
		g_spb.s_free[g_spb.s_nfree] = blkno[a];
		g_spb.s_nfree++;
		g_spb.s_fmod = 1;
	}
	cout << "文件删除成功" << endl;
	return 1;
}

int fopen(char *name, int mode) //读文件名为str的文件入内存i节点，成功则返回内存i节点编号
{
	DirectoryEntry dire[16];
	if (mode != 0 && mode != 1 )
	{
		cout << "打开方式错误" << endl;
		return -1;
	}
	int i,j,c,a,k;
	i = cdir.m_ino;
	j = icopy(i);
	c = inode[j].i_addr[0];
	if (inode[j].i_mode == Inode::IFDIR)
	{
		k = Getbuf(c);
		m_Buf[k].b_flags = Buf::B_READ;
		Strategy(&m_Buf[k], c);
		for (a = 0; a < 16; a++)
		{
			Copy((char*)m_Buf[k].b_addr + a * 32, (char*)&dire[a], 32);
		}
		for (a = 0; a < 16; a++)
		{
			if (strcmp(name, dire[a].m_name) == 0)         //找到该文件
			{
				break;
			}
		}
		if (a == 16)
		{
			cout << "未找到该文件" << endl;
			return 0;
		}
		else
		{
			j = icopy(dire[a].m_ino);
			if (mode == 0)
			{
				inode[j].i_flag = Inode::IREAD;
			}
			else
			{
				inode[j].i_flag = Inode::IWRITE;
			}
			cout << "打开成功，内存i节点编号为" << j << endl;
			return j;
		}
	}
	return 1;
}

int copyin(char *srcpath, char *name)    //将虚拟磁盘外的文件拷贝入虚拟磁盘
{
	FILE *fp1;
	fp1 = fopen(srcpath, "rb+");
	if (fp1 == NULL)
	{
		cout << "未找到该文件！" << endl;
		return 0;
	}
	fcreate(name, 0);
	int fd = fopen(name, 1);
	int size = _filelength(_fileno(fp1));
	int i = size / 512;
	for (int a = 0; a < i; a++)   //指定0号缓存，有不妥之处
	{
		m_Buf[0].b_flags = Buf::B_BUSY;   //确保该缓存这个时候不会被其他函数使用
		nowtime = time(NULL);
		m_Buf[0].b_time = (int)nowtime;
		fread(m_Buf[0].b_addr, 1, 512, fp1);
		flseek(fd, a * 512);
		fwrite(fd, (char*)Buffer[0], 512);
	}
	for (int j = 0; j < 512; j++)
	{
		Buffer[0][j] = 0;
	}
	m_Buf[0].b_flags = Buf::B_BUSY;
	fread(m_Buf[0].b_addr, 1, size % 512, fp1);//多余512字节的剩余字节拷贝
	flseek(fd, i * 512);
	fwrite(fd, (char*)Buffer[0], size % 512);
	flseek(fd, 0);
	m_Buf[0].b_blkno = -1;
	m_Buf[0].b_flags = 0;
	cout << "复制成功" << endl;
	fclose(fp1);
	return 1;
}

int copyout(char *name, char *dstpath)           //将虚拟磁盘内的文件拷贝到虚拟磁盘外
{
	FILE *fp1;
	int fd = fopen(name, 0);
	if (fd == 0)
	{
		return 0;
	}
	fp1 = fopen(dstpath, "wb+");
	int size = inode[fd].i_size;
	int i = size / 512;
	for (int a = 0; a < i; a++) //指定0号缓存，有不妥之处
	{
		m_Buf[0].b_flags = Buf::B_BUSY;   //确保该缓存这个时候不会被其他函数使用
		nowtime = time(NULL);
		m_Buf[0].b_time = (int)nowtime;
		flseek(fd, a * 512);
		fread(fd, (char*)Buffer[0], 512);
		fwrite(m_Buf[0].b_addr, 1, 512, fp1);
	}
	for (int j = 0; j < 512; j++)
	{
		Buffer[0][j] = 0;
	}
	m_Buf[0].b_flags = Buf::B_BUSY;
	flseek(fd, i * 512);
	fread(fd, (char*)Buffer[0], size % 512);                    //多余512字节的剩余字节拷贝
	fwrite(m_Buf[0].b_addr, 1, size % 512, fp1);
	m_Buf[0].b_blkno = -1;
	m_Buf[0].b_flags = 0;
	flseek(fd, 0);
	fclose(fd);
	cout << "复制成功" << endl;
	return 1;
}

int fread(int fd, char *buffer, int length)    //读文件到buffer，由于不是编程环境，buffer缓存这个参数在用户使用的时候无实际意义
{
	if (inode[fd].i_flag != Inode::IREAD && inode[fd].i_flag != Inode::IWRITE)
	{
		cout << "无读权限！" << endl;
		return -1;
	}
	int i,j,k,a;

	i = ((length + inode[fd].i_position) / 512) - (inode[fd].i_position / 512);
	if (((length + inode[fd].i_position) % 512) != 0)
	{
		i++;                //i=要读的总扇区数量
	}
	char str[512];
	memset(str, 0, 512);
	char *p = str;

	a = 0;
	while (a != i)
	{
		j = Getaddr(fd, a + (inode[fd].i_position / 512));

		k = Getbuf(j);
		m_Buf[k].b_flags = Buf::B_READ;
		Strategy(&m_Buf[k], j);

		Copy((char *)m_Buf[k].b_addr, p, 512);
		a++;
	}
	Copy(p, (char*)buffer, length);
	return 1;
}

int fwrite(int fd, char *buffer, int length)//从buffer写文件，由于不是编程环境，buffer缓存这个参数在用户使用的时候无实际意义
{
	if (inode[fd].i_flag != Inode::IWRITE)
	{
		cout << "无写权限！" << endl;
		return -1;
	}
	DiskInode dsk;
	int i = ((length + inode[fd].i_position) / 512) - (inode[fd].i_position / 512);
	int j,k;
	int a = 0,b=0,c=0;
	if (((length + inode[fd].i_position) % 512) != 0) //i为所需要读取的总扇区数，若总的读写大小不是512的整数倍，则b++
	{
		i++;
		b++;
	}
	if (inode[fd].i_position % 512 != 0)          //若初始读取位置不是512的整数倍，则c++
	{
		c++;
	}
	inode[fd].i_size += length;
	while (a != i)
	{
		if (a == (i - 1) && b)
		{
			j = Getaddr(fd, a+ (inode[fd].i_position / 512));
			if (j == -1)    //若沒有記錄該扇区
			{
				if (g_spb.s_nfree > 1)                  //扇区的申请过程
				{
					g_spb.s_nfree--;
					j = g_spb.s_free[g_spb.s_nfree];
					g_spb.s_fmod = 1;
				}
				else if (g_spb.s_free[0] != 2018)
				{
					k = Getbuf(g_spb.s_free[0]);
					m_Buf[k].b_flags = Buf::B_READ;
					Strategy(&m_Buf[k], g_spb.s_free[0]);

					Copy((char*)m_Buf[k].b_addr, (char*)&g_spb.s_free[0], 100 * sizeof(int));

					g_spb.s_nfree = 100;
					g_spb.s_nfree--;
					j = g_spb.s_free[g_spb.s_nfree];
					g_spb.s_fmod = 1;
				}
				else
				{
					cout << "剩余空间不足，无法完成数据写入。" << endl;
					return -1;
				}
				addaddr(fd, j);
			}
			k = Getbuf(j);
			m_Buf[k].b_flags = Buf::B_READ;
			Strategy(&m_Buf[k], j);

			Copy(buffer, (char *)m_Buf[k].b_addr, (length + inode[fd].i_position) % 512);

			m_Buf[k].b_flags = Buf::B_WRITE;
			Strategy(&m_Buf[k], j);

			a++;
		}
		else if (a == 0 && c )
		{
			j = Getaddr(fd, inode[fd].i_position / 512);
			if (j == -1)
			{
				if (g_spb.s_nfree > 1) //扇区的申请过程
				{
					g_spb.s_nfree--;
					j = g_spb.s_free[g_spb.s_nfree];
					g_spb.s_fmod = 1;
				}
				else if (g_spb.s_free[0] != 2018)
				{
					k = Getbuf(g_spb.s_free[0]);
					m_Buf[k].b_flags = Buf::B_READ;
					Strategy(&m_Buf[k], g_spb.s_free[0]);

					Copy((char*)m_Buf[k].b_addr, (char*)&g_spb.s_free[0], 100 * sizeof(int));

					g_spb.s_nfree = 100;
					g_spb.s_nfree--;
					j = g_spb.s_free[g_spb.s_nfree];
					g_spb.s_fmod = 1;
				}
				else
				{
					cout << "剩余空间不足，无法完成数据写入。" << endl;
					return -1;
				}
				addaddr(fd, j);
			}
			k = Getbuf(j);
			m_Buf[k].b_flags = Buf::B_READ;
			Strategy(&m_Buf[k], j);
			Copy(buffer, (char *)m_Buf[k].b_addr + (inode[fd].i_position % 512), 512 - (inode[fd].i_position % 512));
			m_Buf[k].b_flags = Buf::B_WRITE;
			Strategy(&m_Buf[k], j);
			a++;
		}
		else
		{
			j = Getaddr(fd, a+ (inode[fd].i_position / 512));
			if (j == -1)
			{
				if (g_spb.s_nfree > 1) //扇区的申请过程
				{
					g_spb.s_nfree--;
					j = g_spb.s_free[g_spb.s_nfree];
					g_spb.s_fmod = 1;
				}
				else if (g_spb.s_free[0] != 2018)
				{
					k = Getbuf(g_spb.s_free[0]);
					m_Buf[k].b_flags = Buf::B_READ;
					Strategy(&m_Buf[k], g_spb.s_free[0]);
					Copy((char*)m_Buf[k].b_addr, (char*)&g_spb.s_free[0], 100 * sizeof(int));
					g_spb.s_nfree = 100;
					g_spb.s_nfree--;
					j = g_spb.s_free[g_spb.s_nfree];
					g_spb.s_fmod = 1;
				}
				else
				{
					cout << "剩余空间不足，无法完成数据写入。" << endl;
					return -1;
				}
				addaddr(fd, j);
			}
			k = Getbuf(j);
			Copy(buffer, (char *)m_Buf[k].b_addr, 512);
			m_Buf[k].b_flags = Buf::B_WRITE;
			Strategy(&m_Buf[k], j);
			a++;
		}
	}
	memcpy(dskinode.d_addr, inode[fd].i_addr, 10 * sizeof(int));        //对应diskinode内容的修改以及写回
	dskinode.d_size = inode[fd].i_size;
	char* p = (char *)&dskinode;
	j = (inode[fd].i_number / 8) + 3;
	k = Getbuf(j);
	m_Buf[k].b_flags = Buf::B_READ;
	Strategy(&m_Buf[k], j);
	Copy(p, (char *)m_Buf[k].b_addr + (inode[fd].i_number % 8) * 64, 64);
	m_Buf[k].b_flags = Buf::B_WRITE;
	Strategy(&m_Buf[k], j);
	return 1;
}

int addaddr(int fd,int j)
{
	int n, k, page;
	for (n = 0; Getaddr(fd, n) != -1; n++);
	if (n <= 5)
	{
		inode[fd].i_addr[n] = j;            //addr[0~5)
	}
	else if (n <262)                   //addr[6,7]
	{
		if (n == 6 || n == 134)              //addr[6,7]的开始第一块扇区需要特别的申请
		{
			if (g_spb.s_nfree > 1)
			{
				g_spb.s_nfree--;
				page = g_spb.s_free[g_spb.s_nfree];
				g_spb.s_fmod = 1;
			}
			else if (g_spb.s_free[0] != 2018)
			{
				k = Getbuf(g_spb.s_free[0]);
				m_Buf[k].b_flags = Buf::B_READ;
				Strategy(&m_Buf[k], g_spb.s_free[0]);
				Copy((char*)m_Buf[k].b_addr, (char*)&g_spb.s_free[0], 100 * sizeof(int));
				g_spb.s_nfree = 100;
				g_spb.s_nfree--;
				page = g_spb.s_free[g_spb.s_nfree];
				g_spb.s_fmod = 1;
			}
			else
			{
				cout << "剩余空间不足，无法完成数据写入。" << endl;
				return -1;
			}
			k = Getbuf(page);
			m_Buf[k].b_flags = Buf::B_READ;
			Strategy(&m_Buf[k], page);

			Copy((char*)&j, (char*)m_Buf[k].b_addr, 4);
			m_Buf[k].b_flags = Buf::B_WRITE;
			Strategy(&m_Buf[k], page);
			if (n == 6)
			{
				inode[fd].i_addr[6] = page;
			}
			else
			{
				inode[fd].i_addr[7] = page;
			}
		}
		else             //addr[6,7]
		{
			k = Getbuf(inode[fd].i_addr[((n - 6) / 128) + 6]);
			m_Buf[k].b_flags = Buf::B_READ;
			Strategy(&m_Buf[k], inode[fd].i_addr[((n - 6) / 128) + 6]);

			Copy((char*)&j, (char*)m_Buf[k].b_addr + ((n - 6) % 128) * 4, 4);
			m_Buf[k].b_flags = Buf::B_WRITE;
			Strategy(&m_Buf[k], inode[fd].i_addr[((n - 6) / 128) + 6]);
		}
	}
	else
	{
		cout << "存储空间不足！" << endl;
	}

	return 0;
}

void LoadSuperBlock()              //加载系统的时候读spb结构
{
	for (int i = 0; i < 2; i++)
	{
		char* p = (char *)&g_spb + i * 512;
		m_Buf[i].b_flags = Buf::B_READ;
		Strategy(&m_Buf[i], i+1);
		Copy((char *)m_Buf[i].b_addr, p, 512);
	}
}

int Getbuf(int num)        //需要对num扇区进行io操作的时候调用的getbuf函数，返回值为缓存的编号
{
	int i;
	int min = m_Buf[0].b_time;
	for (i = 0; (i < NBUF) && (m_Buf[i].b_blkno != num); i++);
	if (i != NBUF)
	{
		return i;          //找到可以重用的缓存
	}
	else
	{
		for (i = 0; (i < NBUF) && (m_Buf[i].b_blkno != (-1)); i++);
		if (i != NBUF)
		{
			return i;           //找到空缓存
		}
		else
		{
			i = 0;
			if (m_Buf[0].b_flags == Buf::B_BUSY)
			{
				i = 1;
				min = m_Buf[1].b_time;
			}
			for (int a = 0; a < NBUF; a++)
			{
				if (m_Buf[a].b_time < min && m_Buf[a].b_flags!=Buf::B_BUSY)
				{
					i = a;
					min = m_Buf[a].b_time;
				}
			}
			return i;   //根据lru规则找到最久未使用的缓存块
		}
	}
}

int Strategy(Buf* bp, int j)
{
	if (j > 2017)   //大于总扇区数量
	{
		bp->b_flags = Buf::B_ERROR;
		return 0;
	}
	if (bp->b_blkno == j && (bp->b_flags == Buf::B_READ))       //重用机制
	{
		bp->b_flags = Buf::B_DONE;
		nowtime = time(NULL);
		bp->b_time = (int)nowtime;
	}
	else
	{
		bp->b_blkno = j;
		DevStart(bp);
	}
	return 0;
}

int DevStart(Buf* bp)
{
	fseek(fp, (bp->b_blkno - 1) * 512, 0);
	if (bp->b_flags == Buf::B_READ)
	{
		fread(bp->b_addr, 1, 512, fp);
	}
	if (bp->b_flags == Buf::B_WRITE)
	{
		fwrite(bp->b_addr, 1, 512, fp);
	}
	bp->b_flags = Buf::B_DONE;
	nowtime = time(NULL);
	bp->b_time = (int)nowtime;
	return 1;
}
