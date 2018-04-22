#ifndef MYFILESYSTEM_H_INCLUDED
#define MYFILESYSTEM_H_INCLUDED

#include "INode.h"
#include "SuperBlock.h"
#include "DirectoryEntry.h"

#define NBUF 15
#define BUFFER_SIZE 512

void    Initialize();

int     Strategy(Buf* bp, int num);
int     DevStart(Buf* bp);
void    Copy(char *src, char *dst, int count);
int     Getbuf(int num);

void    ls();   //列文件编号为fd的子目录
int     fopen(char *name, int mode);     //读文件名为str的文件入内存i节点，成功则返回内存i节点编号
int     fclose(int fd);   //关闭内存节点编号为fd的文件
int     fread(int fd, char *buffer, int length);  //读内存节点编号为fd的文件length长度至buffer的地址
int     fwrite(int fd, char *buffer, int length);  //从buffer的地址写长度为length的内容到内存节点编号为fd的文件
int     copyin(char *srcpath, char *name);             //从硬盘中srcpath的路径拷贝文件到当前目录，命名为name
int     copyout(char *name, char *dstpath);        //从当前目录拷贝出name的文件到硬盘的dstpath目录中
int     fcreate(char *name, int mode);
int     fdelete(char *name);            //删除名字为name的文件
int     flseek(int fd, int position);   //改变内存文件节点为fd的读写位置i_position为position，成功则返回0

int     icopy(int fd);    //返回文件内存i节点号
int     Getaddr(int fd , int num);   //得到内存文件节点为fd的文件第num块的物理地址
int     addaddr(int fd, int j);    //从spb结构中读取新的可用于当前请求的扇区给请求的文件使用

void    LoadSuperBlock();        //加载系统的时候读spb结构

#endif // MYFILESYSTEM_H_INCLUDED
