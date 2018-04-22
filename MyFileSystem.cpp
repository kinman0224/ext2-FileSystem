#include "MyFileSystem.h"

#include <stdio.h>
#include <iostream>
#include <time.h>
#include <io.h>


using namespace std;

extern FILE *fp;   //ȫ���ļ�ָ��

extern Buf m_Buf[NBUF];           //������ƿ�m_buf[15]
extern unsigned char Buffer[NBUF][BUFFER_SIZE];            //buffer[15][512]����

extern time_t nowtime;

SuperBlock  g_spb;      //superblockȫ�ֱ���
Inode       inode[32];  //�ڴ�inode�ڵ㹲32��

DiskInode   dskinode;   //diskinodeȫ�ֱ���

DirectoryEntry dir;     //ȫ��Ŀ¼��
DirectoryEntry cdir;    //��ǰ��Ŀ¼��

/*==============================class Inode===================================*/
Inode::Inode()
{
	/* ��Inode����ĳ�Ա������ʼ��Ϊ��Чֵ */
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
	 * ���DiskInodeû�й��캯�����ᷢ�����½��Ѳ���Ĵ���
	 * DiskInode��Ϊ�ֲ�����ռ�ݺ���Stack Frame�е��ڴ�ռ䣬����
	 * ��οռ�û�б���ȷ��ʼ�����Ծɱ�������ǰջ���ݣ����ڲ�����
	 * DiskInode�����ֶζ��ᱻ���£���DiskInodeд�ص�������ʱ������
	 * ����ǰջ����һͬд�أ�����д�ؽ������Ī����������ݡ�
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
 *   FileSystem �����Ķ��x
 *
 */

void Copy(char *src, char *dst, int count)        //������������src�ڴ濽��count���ֽڵ�dst�ڴ�
{
	while (count--)
	{
		*dst = *src;
		dst++;
		src++;
	}
	return;
}

int Getaddr(int fd, int num)    //ͨ���ڴ�ڵ����num�����ݿ�õ��ÿ��ַ
{
	int k;
	if (num <= 5)      //addr[0~5]
	{
		return inode[fd].i_addr[num];
	}
	else if (num > 5 && num <= 261)       //addr[6,7]һ��������
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
		cout << "��������ȷ�����" << endl;
		return -2;
	}
}

void ls()              //��Ŀ¼����
{
	DirectoryEntry direc[16];
	int i,j,k,a;
	i = cdir.m_ino;
	j = icopy(i);
	if (inode[j].i_mode == Inode::IFDIR)
	{
		k = Getbuf(inode[j].i_addr[0]);
		m_Buf[k].b_flags = Buf::B_READ;
		Strategy(&m_Buf[k], inode[j].i_addr[0]);           //���뵱ǰĿ¼��ռ�ݵ�����

		for (a = 0; a < 16; a++)
		{
			Copy((char*)m_Buf[k].b_addr + a * 32, (char*)&direc[a], 32);   //��ȡ16��directoryentry
		}
		for (a = 1; a < 16; a++)          //��һ��directoryentry����Ŀ¼�����г�
		{
			if (direc[a].m_ino != -1)
			{
				cout << direc[a].m_name << "     ";
			}
		}
	}
	cout << endl;
}

int fclose(int fd)                     //�ر��ļ������޸Ĺ���ʱ����Ҫд�ص������Ѿ���fwrite��ʵ��
{
	if (inode[fd].i_number == -1)
	{
		cout << "�޴��ļ���" << endl;
		return 0;
	}
	inode[fd].i_number = -1;
	cout << "�ر��ļ��ɹ�" << endl;
	return 1;
}

int icopy(int fd)   //��������Ҫ�õ���i�ڵ�ṹ�������ڴ���
{
	DiskInode dInode;
	int i, fd1, j;

	for (i = 0; i < 32; i++)
	{
		if (inode[i].i_number == fd)      //�Ѿ����ڴ��У�ֱ�ӷ��ر��
		{
			return i;
		}
	}
	for (i = 0; i < 32; i++)
		{
			if (inode[i].i_number == -1)     //�ҵ������ڴ�i�ڵ�
			{
				break;
			}
		}
		if (i == 32)
		{
			cout << "��ǰ���ļ��������࣬������Ҫ�ر��ļ����ڴ�i�ڵ��ţ��ر��ļ���" << endl;
			cin >> fd1;
			i = fclose(fd1);
			return -1;
		}
		j = (fd / 8) + 3;
		int k = Getbuf(j);

		m_Buf[k].b_flags = Buf::B_READ;   //�Ӵ�����diskinode��ȡ���ڸ��ļ��ĸ�����Ϣ
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

int flseek(int fd, int position)         //�ı��ļ���дָ��
{
	inode[fd].i_position=position;
	return 0;
}

int fcreate(char *name, int mode)   //�ڸ�Ŀ¼�´�����Ϊname���ļ�
{
	int blkno,di,j,k,b;
	DirectoryEntry dire[16];
	if (g_spb.s_nfree > 0)              //ʹ��һ�������̿�
	{
		g_spb.s_nfree--;
		blkno = g_spb.s_free[g_spb.s_nfree];
		g_spb.s_fmod = 1;
	}
	else if (g_spb.s_free[0] != 2018)             //nfree==0��ʱ�������s_free���㷨
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
		cout << "ʣ��ռ䲻�㣬�޷�����ļ�������" << endl;
		return -1;
	}
	if (mode != 1 && mode != 0)
	{
		cout << "��������ȷ���ļ����ͺ����ԡ�" << endl;
		return -2;
	}
	if (g_spb.s_ninode >= 0)
	{
		di = g_spb.s_inode[--g_spb.s_ninode];
		g_spb.s_fmod = 1;
	}
	else
	{
		cout << "�ļ�������������ɾ������Ҫ���ļ�" << endl;
		return -3;
	}
	//int a = 0;
	j = (cdir.m_ino / 8) + 3;          //����Ϊ��ȡ��ǰ��Ŀ¼�ļ���д��name��Ŀ¼�ļ��е��㷨
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
		cout << "��Ŀ¼�µ��ļ���������ɾ�����ļ���Ŀ¼������" << endl;
		return -1;
	}
	dire[b].m_ino = di;
	strcpy(dire[b].m_name , name);
	Copy((char*)&dire[b], (char*)m_Buf[k].b_addr + 32 * b, 32);
	m_Buf[k].b_flags = Buf::B_WRITE;
	Strategy(&m_Buf[k], j);
	if (mode == 1)               //���￪ʼ�޸�dskinode���󣬲�����д��diskinode�����е�ָ������di��diskinode��
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
	cout << "�ļ������ɹ�" << endl;
	return di;
}

int fdelete(char *name)
{
	DirectoryEntry dire[16];
	int i, j, k, a,c;
	int blkno[100] = { 0 };                //blkno�������ڴ�����Ի���ͷŵ�����
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
	if (strcmp(name, dire[a].m_name) == 0)             //�ҵ����ļ�
	{
	    break;
	}
	if (a == 16)
	{
		cout << "δ�ҵ����ļ�" << endl;
		return 0;
	}
	dire[a].m_ino = -1;               //��ӦĿ¼����Ϊ��ʼֵ
	for (j = 0; j < DirectoryEntry::DIRSIZ; j++)
	{
		dire[a].m_name[j] = '\0';
	}
	Copy((char*)&dire[a], (char*)m_Buf[k].b_addr + 32 * a, 32);
	m_Buf[k].b_flags = Buf::B_WRITE;
	Strategy(&m_Buf[k], c);                //Ŀ¼����д��
	i = icopy(dire[a].m_ino);
	j = (dire[a].m_ino / 8) + 3;
	k = Getbuf(j);
	m_Buf[k].b_flags = Buf::B_READ;
	Strategy(&m_Buf[k], j);              //��������diskinode������������������
	char *p = (char *)&dskinode;          //��dskinode��ַ����pָ��
	for (int b = 0;Getaddr(a,b) != -1; b++ )
	{
		blkno[b] = Getaddr(a, b);
	}
	for (int n = 0; n < 10; n++ )            //dskinode�ÿգ�׼��д��Ӳ��Ҫɾ���Ǹ��ļ���diskinode����
	{
		dskinode.d_addr[n] = -1;
	}
	dskinode.d_size = 0;
	dskinode.d_mode = 0;
	dskinode.d_atime = 0;
	dskinode.d_mtime = 0;
	Copy(p, (char*)m_Buf[k].b_addr+(i%8)*64, sizeof(DiskInode));
	m_Buf[k].b_flags = Buf::B_WRITE;
	Strategy(&m_Buf[k], j);                         //дdiskinode��Ӳ��
	for (a = 0; a < 100&&blkno[a]!=0; a++)       //�ͷſ����̿�
	{
		g_spb.s_free[g_spb.s_nfree] = blkno[a];
		g_spb.s_nfree++;
		g_spb.s_fmod = 1;
	}
	cout << "�ļ�ɾ���ɹ�" << endl;
	return 1;
}

int fopen(char *name, int mode) //���ļ���Ϊstr���ļ����ڴ�i�ڵ㣬�ɹ��򷵻��ڴ�i�ڵ���
{
	DirectoryEntry dire[16];
	if (mode != 0 && mode != 1 )
	{
		cout << "�򿪷�ʽ����" << endl;
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
			if (strcmp(name, dire[a].m_name) == 0)         //�ҵ����ļ�
			{
				break;
			}
		}
		if (a == 16)
		{
			cout << "δ�ҵ����ļ�" << endl;
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
			cout << "�򿪳ɹ����ڴ�i�ڵ���Ϊ" << j << endl;
			return j;
		}
	}
	return 1;
}

int copyin(char *srcpath, char *name)    //�������������ļ��������������
{
	FILE *fp1;
	fp1 = fopen(srcpath, "rb+");
	if (fp1 == NULL)
	{
		cout << "δ�ҵ����ļ���" << endl;
		return 0;
	}
	fcreate(name, 0);
	int fd = fopen(name, 1);
	int size = _filelength(_fileno(fp1));
	int i = size / 512;
	for (int a = 0; a < i; a++)   //ָ��0�Ż��棬�в���֮��
	{
		m_Buf[0].b_flags = Buf::B_BUSY;   //ȷ���û������ʱ�򲻻ᱻ��������ʹ��
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
	fread(m_Buf[0].b_addr, 1, size % 512, fp1);//����512�ֽڵ�ʣ���ֽڿ���
	flseek(fd, i * 512);
	fwrite(fd, (char*)Buffer[0], size % 512);
	flseek(fd, 0);
	m_Buf[0].b_blkno = -1;
	m_Buf[0].b_flags = 0;
	cout << "���Ƴɹ�" << endl;
	fclose(fp1);
	return 1;
}

int copyout(char *name, char *dstpath)           //����������ڵ��ļ����������������
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
	for (int a = 0; a < i; a++) //ָ��0�Ż��棬�в���֮��
	{
		m_Buf[0].b_flags = Buf::B_BUSY;   //ȷ���û������ʱ�򲻻ᱻ��������ʹ��
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
	fread(fd, (char*)Buffer[0], size % 512);                    //����512�ֽڵ�ʣ���ֽڿ���
	fwrite(m_Buf[0].b_addr, 1, size % 512, fp1);
	m_Buf[0].b_blkno = -1;
	m_Buf[0].b_flags = 0;
	flseek(fd, 0);
	fclose(fd);
	cout << "���Ƴɹ�" << endl;
	return 1;
}

int fread(int fd, char *buffer, int length)    //���ļ���buffer�����ڲ��Ǳ�̻�����buffer��������������û�ʹ�õ�ʱ����ʵ������
{
	if (inode[fd].i_flag != Inode::IREAD && inode[fd].i_flag != Inode::IWRITE)
	{
		cout << "�޶�Ȩ�ޣ�" << endl;
		return -1;
	}
	int i,j,k,a;

	i = ((length + inode[fd].i_position) / 512) - (inode[fd].i_position / 512);
	if (((length + inode[fd].i_position) % 512) != 0)
	{
		i++;                //i=Ҫ��������������
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

int fwrite(int fd, char *buffer, int length)//��bufferд�ļ������ڲ��Ǳ�̻�����buffer��������������û�ʹ�õ�ʱ����ʵ������
{
	if (inode[fd].i_flag != Inode::IWRITE)
	{
		cout << "��дȨ�ޣ�" << endl;
		return -1;
	}
	DiskInode dsk;
	int i = ((length + inode[fd].i_position) / 512) - (inode[fd].i_position / 512);
	int j,k;
	int a = 0,b=0,c=0;
	if (((length + inode[fd].i_position) % 512) != 0) //iΪ����Ҫ��ȡ���������������ܵĶ�д��С����512������������b++
	{
		i++;
		b++;
	}
	if (inode[fd].i_position % 512 != 0)          //����ʼ��ȡλ�ò���512������������c++
	{
		c++;
	}
	inode[fd].i_size += length;
	while (a != i)
	{
		if (a == (i - 1) && b)
		{
			j = Getaddr(fd, a+ (inode[fd].i_position / 512));
			if (j == -1)    //���]��ӛ�ԓ����
			{
				if (g_spb.s_nfree > 1)                  //�������������
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
					cout << "ʣ��ռ䲻�㣬�޷��������д�롣" << endl;
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
				if (g_spb.s_nfree > 1) //�������������
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
					cout << "ʣ��ռ䲻�㣬�޷��������д�롣" << endl;
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
				if (g_spb.s_nfree > 1) //�������������
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
					cout << "ʣ��ռ䲻�㣬�޷��������д�롣" << endl;
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
	memcpy(dskinode.d_addr, inode[fd].i_addr, 10 * sizeof(int));        //��Ӧdiskinode���ݵ��޸��Լ�д��
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
		if (n == 6 || n == 134)              //addr[6,7]�Ŀ�ʼ��һ��������Ҫ�ر������
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
				cout << "ʣ��ռ䲻�㣬�޷��������д�롣" << endl;
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
		cout << "�洢�ռ䲻�㣡" << endl;
	}

	return 0;
}

void LoadSuperBlock()              //����ϵͳ��ʱ���spb�ṹ
{
	for (int i = 0; i < 2; i++)
	{
		char* p = (char *)&g_spb + i * 512;
		m_Buf[i].b_flags = Buf::B_READ;
		Strategy(&m_Buf[i], i+1);
		Copy((char *)m_Buf[i].b_addr, p, 512);
	}
}

int Getbuf(int num)        //��Ҫ��num��������io������ʱ����õ�getbuf����������ֵΪ����ı��
{
	int i;
	int min = m_Buf[0].b_time;
	for (i = 0; (i < NBUF) && (m_Buf[i].b_blkno != num); i++);
	if (i != NBUF)
	{
		return i;          //�ҵ��������õĻ���
	}
	else
	{
		for (i = 0; (i < NBUF) && (m_Buf[i].b_blkno != (-1)); i++);
		if (i != NBUF)
		{
			return i;           //�ҵ��ջ���
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
			return i;   //����lru�����ҵ����δʹ�õĻ����
		}
	}
}

int Strategy(Buf* bp, int j)
{
	if (j > 2017)   //��������������
	{
		bp->b_flags = Buf::B_ERROR;
		return 0;
	}
	if (bp->b_blkno == j && (bp->b_flags == Buf::B_READ))       //���û���
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
