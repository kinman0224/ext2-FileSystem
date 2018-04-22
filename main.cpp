#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <windows.h>

#include <cctype>
#include <algorithm>

#include "MyFileSystem.h"

extern Buf m_Buf[NBUF];           //缓存控制块m_buf[15]
extern FILE *fp;   //全局文件指针
extern unsigned char Buffer[NBUF][BUFFER_SIZE];            //buffer[15][512]缓存
extern time_t nowtime;

extern SuperBlock  g_spb;    //superblock全局变量
extern Inode       inode[32];   //内存inode节点共32个
extern DiskInode   dskinode;//diskinode全局变量

extern DirectoryEntry dir; //全局目录项
extern DirectoryEntry cdir; //当前打开目录项

char    buffer[8192];

using namespace std;

void InitFileSystem()
{
    string select;

    while (true)
    {
        cout << "Have to initialize the system? Input[y/n]: ";
        getline(cin, select);

        transform(select.begin(), select.end(), select.begin(), ::tolower);
        if (select == "y")
        {
            Initialize();
            cout << "initialization finished!" << endl;
            break;
        }
        else
        if (select == "n")
        {
            break;
        }
        else
        {
            cout << "Input[y/n]" << endl << endl;
        }
    }

}

void Run_System()
{
    DirectoryEntry direc[15];

    fp = fopen("MyDisk.img", "rb+");
	for (int i = 0; i < NBUF; i++)
	{
		m_Buf[i].b_addr = Buffer[i];     //初始化buf
	}
	LoadSuperBlock();            //读superblock
	icopy(0);                       //读0号i节点

	strcpy(cdir.m_name, "/");        //设置当前工作目录为根目录
	cdir.m_ino = 0;

	string cmd;
    int i, j, k;

	while (true)
	{
		string com[256];

		cout << "[MyFileSystem]:" << cdir.m_name << " $ ";

		getline(cin, cmd);

		int pos = 0;
		for (unsigned int s_pos = 0 ; s_pos < cmd.length(); s_pos++)
		{
			if (cmd[s_pos] != ' ')
				com[pos] += cmd[s_pos];
			else
				pos++;
		}

		if (com[0] == "help")
		{
			cout << "input this command." << endl << endl;
		}
		else
		if (com[0] == "ls")
		{
            ls();
		}
        else
        if (com[0] == "cd")
        {
            if (com[1] == "..") //返回上一級目錄
            {
                j = icopy(cdir.m_ino);                //从当前目录的对应扇区的第1个directoryentry结构中读到父目录的数据
				k = Getbuf(inode[j].i_addr[0]);
				m_Buf[k].b_flags = Buf::B_READ;
                Strategy(&m_Buf[k], inode[j].i_addr[0]);
				Copy((char*)m_Buf[k].b_addr, (char*)&dir, 32);

				cdir = dir;              //变换当前工作目录
            }
            else
            {
                j = icopy(cdir.m_ino);
                k = Getbuf(inode[j].i_addr[0]);
                m_Buf[k].b_flags = Buf::B_READ;
				Strategy(&m_Buf[k], inode[j].i_addr[0]);

                for (int b = 1; b < 16; b++)
				{
					Copy((char*)m_Buf[k].b_addr + b*32, (char*)&direc[b-1], 32);
				}
				i = 0;

				while (com[1] != direc[i].m_name)
				{
					i++;
				}

				if (i < 16)
				{
					cdir = direc[i];
				}
				else
				{
					cout << "Cannot find the directory! "  << com[1] << endl;
				}
            }
        }
        else
        if (com[0] == "fopen")
        {
            char filename[256];
            int mode;

            strcpy(filename, com[1].c_str());
            mode     = atoi(com[2].c_str());

            fopen(filename, mode);
        }
        else
        if (com[0] == "fclose")
        {
            int fd = atoi(com[1].c_str());
            fclose(fd);
        }
        else
        if (com[0] == "fread")
        {
            int fd, length;

            fd = atoi(com[1].c_str());
            length = atoi(com[2].c_str());

            fread(fd, buffer, length);
        }
        else
        if (com[0] == "fwrite")
        {
            int fd, length;

            fd = atoi(com[1].c_str());
            length = atoi(com[2].c_str());

            fwrite(fd, buffer, length);
        }
        else
        if (com[0] == "fcreate")
        {
            char filename[256];
            int mode;

            strcpy(filename, com[1].c_str());
            mode = atoi(com[2].c_str());

            fcreate(filename, mode);
        }
        else
        if (com[0] == "fdelete")
        {
            char filename[256];
            strcpy(filename, com[1].c_str());

            fdelete(filename);
        }
        else
        if (com[0] == "fseek")
        {
            int fd       = atoi(com[1].c_str());
            int position = atoi(com[2].c_str());

            flseek(fd, position);
        }
        else
        if (com[0] == "copyin")
        {
            char filepath[100], filename[256];

            strcpy(filepath, com[1].c_str());
            strcpy(filename, com[2].c_str());

            copyin(filepath, filename);
        }
        else
        if (com[0] == "copyout")
        {
            char filepath[100], filename[256];

            strcpy(filename, com[1].c_str());
            strcpy(filepath, com[2].c_str());

            copyout(filename, filepath);
        }
        else
        if (com[0] == "shutdown")
        {
            if (g_spb.s_fmod == 1)          //若superblock被修改过，将其写回磁盘
			{
				for (int i = 0; i < 2; i++)
				{
					char* p = (char *)&g_spb + i * 512;
					Copy(p, (char *)m_Buf[i].b_addr, 512);
					m_Buf[i].b_flags = Buf::B_WRITE;
					Strategy(&m_Buf[i], i + 1);
				}
			}
			if (fp)
			{
				fclose(fp);
			}
			break;
        }
	}
}

int main()
{

    InitFileSystem();

    cout << endl << "login in the system, please wait..." << endl << endl;

    Sleep(1000);
    system("cls");

    Run_System();

    return 0;
}
