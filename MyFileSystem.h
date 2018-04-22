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

void    ls();   //���ļ����Ϊfd����Ŀ¼
int     fopen(char *name, int mode);     //���ļ���Ϊstr���ļ����ڴ�i�ڵ㣬�ɹ��򷵻��ڴ�i�ڵ���
int     fclose(int fd);   //�ر��ڴ�ڵ���Ϊfd���ļ�
int     fread(int fd, char *buffer, int length);  //���ڴ�ڵ���Ϊfd���ļ�length������buffer�ĵ�ַ
int     fwrite(int fd, char *buffer, int length);  //��buffer�ĵ�ַд����Ϊlength�����ݵ��ڴ�ڵ���Ϊfd���ļ�
int     copyin(char *srcpath, char *name);             //��Ӳ����srcpath��·�������ļ�����ǰĿ¼������Ϊname
int     copyout(char *name, char *dstpath);        //�ӵ�ǰĿ¼������name���ļ���Ӳ�̵�dstpathĿ¼��
int     fcreate(char *name, int mode);
int     fdelete(char *name);            //ɾ������Ϊname���ļ�
int     flseek(int fd, int position);   //�ı��ڴ��ļ��ڵ�Ϊfd�Ķ�дλ��i_positionΪposition���ɹ��򷵻�0

int     icopy(int fd);    //�����ļ��ڴ�i�ڵ��
int     Getaddr(int fd , int num);   //�õ��ڴ��ļ��ڵ�Ϊfd���ļ���num��������ַ
int     addaddr(int fd, int j);    //��spb�ṹ�ж�ȡ�µĿ����ڵ�ǰ�����������������ļ�ʹ��

void    LoadSuperBlock();        //����ϵͳ��ʱ���spb�ṹ

#endif // MYFILESYSTEM_H_INCLUDED
