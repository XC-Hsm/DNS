/***************************************************************
* @file		control.c
* @brief	�����в���������־�����������������
* @author	�������󸢲���������
* @version	1.0.0
* @date		2021-06-01
***************************************************************/
#ifndef	_CRT_SECURE_NO_WARNINGS				/*ȡ��VS���ֺ������뾯��*/
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "control.h"

int logNum = 1;								/*��־���*/
char gDBtxt[100] = "dnsrelay.txt";			/*���ݿ��ļ�·��*/
char tmpgDBtxt[100] = "tmpdnsrelay.txt";	/*���ݿ���ʱ�ļ�·��*/
char addrDNSserv[16] = "10.3.9.45";			/*DNS��������ַ*/
int gDebugLevel = 0;						/*���Եȼ�*/

/**************************************************************
* @brief	���ֽڸ�ʽ�������������
			���ҽ������Եȼ�Ϊ2ʱ�����
* @param	const unsigned char*	buf		������ָ��
* @param	int						bufSize	��������С
* @return	void
**************************************************************/
void DebugBuffer(const unsigned char* buf, int bufSize) {
	/*���ҽ������Եȼ�Ϊ2ʱ�����*/
	if (gDebugLevel < 2)return;
	char isEnd = 0;
	if (bufSize > MAX_BUFSIZE) {
		DebugPrintf("***************************************************");
		DebugPrintf("ERROR	DebugBuffer() failed, bufSize too big: %d>%d", bufSize, MAX_BUFSIZE);
		DebugPrintf("***************************************************");
	}
	else {
		for (int i = 0; i < bufSize; ++i) {
			DebugPrintf("%02x ", buf[i]);
			isEnd = 0;
			if (i % 16 == 15) {
				DebugPrintf("\n");
				isEnd = 1;
			}
		}
		if (!isEnd)
			DebugPrintf("\n");
	}
}

/**************************************************************
* @brief	����DEBUG�����Ϣ
			���ҽ������Եȼ�Ϊ2ʱ�����
* @param	const char*	cmd	���ģʽ��
* @param	const char*	...	�ɱ䳤�����б�
* @return	void
**************************************************************/
void DebugPrintf(const char* cmd, ...) {
	/*���ҽ������Եȼ�Ϊ2ʱ�����*/
	if (gDebugLevel < 2) return; 
	va_list args;
	va_start(args, cmd);
	vprintf(cmd, args);
	va_end(args);
}

/**************************************************************
* @brief	��ջ�����
* @param	unsigned char*	buf		������ָ��
* @param	int				bufSize	��������С
* @return	void
**************************************************************/
void ClearBuffer(unsigned char* buf, int bufSize) {
	if (bufSize > MAX_BUFSIZE) {
		DebugPrintf("***************************************************");
		DebugPrintf("ERROR	ClearBuffer() failed, bufSize too big: %d>%d\n", bufSize, MAX_BUFSIZE);
		DebugPrintf("***************************************************");
	}

	else if (bufSize < 0) {
		DebugPrintf("***************************************************");
		DebugPrintf("ERROR	ClearBuffer() failed, bufSize error: %d\n", bufSize);
		DebugPrintf("***************************************************");
	}
	else
		memset(buf, 0, bufSize);
}

/**************************************************************
* @brief	�����в�����������ģʽ����
			���õ��Լ���
			����DNS������IP��ַ
			�������ݿ��ļ���ַ
* @param	int		argc	��������
* @param	char*	argv[]	�����б�
* @return	void
**************************************************************/
extern int dealOpts(int argc, char* argv[]) {
	int i = 1;				/*�ӵ�1��������ʼ*/
	int ipInt[4];			/*�洢DNS������IP��ַ*/
	int stri = 0;			/*DNS������IP��ַ�ַ��������±�*/
	int zeroValid = 0;		/*��ʶ����0�Ƿ���Ч*/
	
	if (i >= argc)return 1;	/*ʶ�����,��ȷ*/

	/*��ȡ���Լ���*/
	if (!strcmp(argv[i], "-dd")) {
		gDebugLevel = 2;
		++i;
	}
	else if (!strcmp(argv[i], "-d")) {
		gDebugLevel = 1;
		++i;
	}

	if (i >= argc)return 1;	/*ʶ�����,��ȷ*/

	/*��ȡDNS��������IP��ַ*/
	/*�򵥼��һ��ip��ַ*/
	if ((4 == sscanf(argv[i], "%d.%d.%d.%d", ipInt + 0, ipInt + 1, ipInt + 2, ipInt + 3))
		&& ipInt[0] < 256 && ipInt[1] < 256 && ipInt[2] < 256 && ipInt[3] < 256) { 
		/*����4�����֣�ת��Ϊ�ַ�����ʽ*/
		for (int ipInti = 0; ipInti < 4; ipInti++) {
			if (stri) { addrDNSserv[stri++] = '.'; }/*���˿�ͷ���ӵ�*/
			zeroValid = 0;
			for (int div = 100; div > 0; div /= 10) {		/*����ÿһλ*/
				addrDNSserv[stri] = ipInt[ipInti] / div;	/*���㵱ǰλ����ֵ*/
				if (zeroValid || addrDNSserv[stri]) {		/*�����ǰֵ����Ч0��Ϊ0*/
					addrDNSserv[stri++] += 48;		/*ת��Ϊ�ַ�*/
					ipInt[ipInti] %= div;			/*ȥ�����λ*/
					zeroValid = 1;					/*������0����Ч����Ҫд���ַ���*/
				}
			}
			if (!zeroValid) { addrDNSserv[stri++] = '0'; }	/*����0����������һ��0*/
		}
		addrDNSserv[stri] = '\0';
		++i; /*��һ�������в���*/
	}

	if (i >= argc)return 1;	/*ʶ�����,��ȷ*/

	/*�������ݿ��ļ���ַ*/
	if (!strcmp(argv[i] + strlen(argv[i]) - 4, ".txt")) {
		strcpy(gDBtxt, argv[i]);		/*���ݿ��ļ���ַ*/
		strcpy(tmpgDBtxt + 3, argv[i]);	/*���ݿ���ʱ�ļ���ַ*/
		++i;							/*��һ�������в���*/
	}

	if (i >= argc)return 1;				/*�����������в���,���سɹ�*/
	else return 0;						/*û������,����ʧ��*/
}

/**************************************************************
* @brief	һ��DEBUG�����Ϣ
			���ҽ������Եȼ����ڵ���1ʱ�����
* @param	const char*	cmd	���ģʽ��
* @param	const char*	...	�ɱ䳤�����б�
* @return	void
**************************************************************/
void debugPrintf(const char* cmd, ...) {
	/*���ҽ������Եȼ����ڵ���1ʱ��ִ��*/
	if (gDebugLevel < 1) return; 
	va_list args;
	va_start(args, cmd);
	printf("%d", logNum);			/*������*/
	logNum++;
	logNum = logNum % MAX_LOGNUM;	/*ѭ�����*/
	time_t t;
	struct tm* lt;    
	time(&t);						/*��ȡʱ���*/
	lt = localtime(&t);				/*תΪtm�ṹ��*/
	printf("	%d/%d/%d %d:%d:%d   	", lt->tm_year + 1900, lt->tm_mon, lt->tm_mday,
		lt->tm_hour, lt->tm_min, lt->tm_sec);/*�����־*/
	vprintf(cmd, args);
	va_end(args);
}