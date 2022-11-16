/***************************************************************
* @file		control.h
* @brief	�����в���������־�����������������
* @author	�������󸢲���������
* @version	1.0.0
* @date		2021-06-01
***************************************************************/
#pragma once
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

#define MAX_BUFSIZE 512				/*��������С����*/
#define MAX_IP_BUFSIZE 16			/*IP����󳤶�*/
#define MAX_DOMAINNAME 100			/*��������󳤶�*/
#define TTL 120						/*TTL�ĳ�ʼֵ*/
#define TIMEOUT 3					/*�ͻ�������ʱʱ��*/
#define MAX_LOGNUM 100000			/*��־�����󳤶�*/
typedef enum { dgram_arrival, nothing } event_type; /*�¼�����*/
extern char gDBtxt[100];			/*�洢���ݿ��ļ���*/
extern char tmpgDBtxt[100];			/*�洢���ݿ���ʱ�ļ���*/
extern int abc;
extern char addrDNSserv[16];		/*DNS��������ַ*/
int gDebugLevel;					/*���Եȼ�*/

/**************************************************************
* @brief	���ֽڸ�ʽ�������������
			���ҽ������Եȼ�Ϊ2ʱ�����
* @param	const unsigned char*	buf		������ָ��
* @param	int						bufSize	��������С
* @return	void
**************************************************************/
extern void DebugBuffer(const unsigned char* buf, int bufSize);

/**************************************************************
* @brief	����DEBUG�����Ϣ
			���ҽ������Եȼ�Ϊ2ʱ�����
* @param	const char*	cmd	���ģʽ��
* @param	const char*	...	�ɱ䳤�����б�
* @return	void
**************************************************************/
extern void DebugPrintf(const char* cmd, ...);

/**************************************************************
* @brief	��ջ�����
* @param	unsigned char*	buf		������ָ��
* @param	int				bufSize	��������С
* @return	void
**************************************************************/
extern void ClearBuffer(unsigned char* buf, int bufSize);

/**************************************************************
* @brief	�����в�����������ģʽ����
			���õ��Լ���
			����DNS������IP��ַ
			�������ݿ��ļ���ַ
* @param	int		argc	��������
* @param	char*	argv[]	�����б�
* @return	void
**************************************************************/
extern int dealOpts(int argc, char* argv[]);

/**************************************************************
* @brief	һ��DEBUG�����Ϣ
			���ҽ������Եȼ����ڵ���1ʱ�����
* @param	const char*	cmd	���ģʽ��
* @param	const char*	...	�ɱ䳤�����б�
* @return	void
**************************************************************/
extern void debugPrintf(const char* cmd, ...);