#ifndef DIRECTORYENTRY_H_INCLUDED
#define DIRECTORYENTRY_H_INCLUDED

class DirectoryEntry
{
	/* static members */
public:
	static const int DIRSIZ = 28;	/* 目录项中路径部分的最大字符串长度 */

	/* Functions */
public:
	/* Constructors */
	DirectoryEntry();
	/* Destructors */
	~DirectoryEntry();

	/* Members */
public:
	int m_ino;		/* 目录项中diskinode编号部分 */
	char m_name[DIRSIZ];	/* 文件名部分 */
};

#endif // DIRECTORYENTRY_H_INCLUDED
