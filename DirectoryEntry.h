#ifndef DIRECTORYENTRY_H_INCLUDED
#define DIRECTORYENTRY_H_INCLUDED

class DirectoryEntry
{
	/* static members */
public:
	static const int DIRSIZ = 28;	/* Ŀ¼����·�����ֵ�����ַ������� */

	/* Functions */
public:
	/* Constructors */
	DirectoryEntry();
	/* Destructors */
	~DirectoryEntry();

	/* Members */
public:
	int m_ino;		/* Ŀ¼����diskinode��Ų��� */
	char m_name[DIRSIZ];	/* �ļ������� */
};

#endif // DIRECTORYENTRY_H_INCLUDED
