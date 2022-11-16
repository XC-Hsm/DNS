/***************************************************************
* @file		control.c
* @brief	命令行参数处理、日志输出、缓冲区处理函数
* @author	韩世民、左涪波、张渝鹏
* @version	1.0.0
* @date		2021-06-01
***************************************************************/
#ifndef	_CRT_SECURE_NO_WARNINGS				/*取消VS部分函数编译警告*/
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "control.h"

int logNum = 1;								/*日志序号*/
char gDBtxt[100] = "dnsrelay.txt";			/*数据库文件路径*/
char tmpgDBtxt[100] = "tmpdnsrelay.txt";	/*数据库临时文件路径*/
char addrDNSserv[16] = "10.3.9.45";			/*DNS服务器地址*/
int gDebugLevel = 0;						/*调试等级*/

/**************************************************************
* @brief	以字节格式输出缓冲区内容
			当且仅当调试等级为2时才输出
* @param	const unsigned char*	buf		缓冲区指针
* @param	int						bufSize	缓冲区大小
* @return	void
**************************************************************/
void DebugBuffer(const unsigned char* buf, int bufSize) {
	/*当且仅当调试等级为2时才输出*/
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
* @brief	二级DEBUG输出信息
			当且仅当调试等级为2时才输出
* @param	const char*	cmd	输出模式串
* @param	const char*	...	可变长参数列表
* @return	void
**************************************************************/
void DebugPrintf(const char* cmd, ...) {
	/*当且仅当调试等级为2时才输出*/
	if (gDebugLevel < 2) return; 
	va_list args;
	va_start(args, cmd);
	vprintf(cmd, args);
	va_end(args);
}

/**************************************************************
* @brief	清空缓冲区
* @param	unsigned char*	buf		缓冲区指针
* @param	int				bufSize	缓冲区大小
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
* @brief	命令行参数处理，操作模式设置
			设置调试级别
			设置DNS服务器IP地址
			设置数据库文件地址
* @param	int		argc	参数个数
* @param	char*	argv[]	参数列表
* @return	void
**************************************************************/
extern int dealOpts(int argc, char* argv[]) {
	int i = 1;				/*从第1个参数开始*/
	int ipInt[4];			/*存储DNS服务器IP地址*/
	int stri = 0;			/*DNS服务器IP地址字符串数组下标*/
	int zeroValid = 0;		/*标识后续0是否有效*/
	
	if (i >= argc)return 1;	/*识别结束,正确*/

	/*获取调试级别*/
	if (!strcmp(argv[i], "-dd")) {
		gDebugLevel = 2;
		++i;
	}
	else if (!strcmp(argv[i], "-d")) {
		gDebugLevel = 1;
		++i;
	}

	if (i >= argc)return 1;	/*识别结束,正确*/

	/*获取DNS服务器的IP地址*/
	/*简单检查一下ip地址*/
	if ((4 == sscanf(argv[i], "%d.%d.%d.%d", ipInt + 0, ipInt + 1, ipInt + 2, ipInt + 3))
		&& ipInt[0] < 256 && ipInt[1] < 256 && ipInt[2] < 256 && ipInt[3] < 256) { 
		/*遍历4个数字，转存为字符串格式*/
		for (int ipInti = 0; ipInti < 4; ipInti++) {
			if (stri) { addrDNSserv[stri++] = '.'; }/*除了开头都加点*/
			zeroValid = 0;
			for (int div = 100; div > 0; div /= 10) {		/*遍历每一位*/
				addrDNSserv[stri] = ipInt[ipInti] / div;	/*计算当前位的数值*/
				if (zeroValid || addrDNSserv[stri]) {		/*如果当前值是有效0或不为0*/
					addrDNSserv[stri++] += 48;		/*转换为字符*/
					ipInt[ipInti] %= div;			/*去掉最高位*/
					zeroValid = 1;					/*后续的0都有效，需要写入字符串*/
				}
			}
			if (!zeroValid) { addrDNSserv[stri++] = '0'; }	/*对于0的数，补充一个0*/
		}
		addrDNSserv[stri] = '\0';
		++i; /*下一个命令行参数*/
	}

	if (i >= argc)return 1;	/*识别结束,正确*/

	/*设置数据库文件地址*/
	if (!strcmp(argv[i] + strlen(argv[i]) - 4, ".txt")) {
		strcpy(gDBtxt, argv[i]);		/*数据库文件地址*/
		strcpy(tmpgDBtxt + 3, argv[i]);	/*数据库临时文件地址*/
		++i;							/*下一个命令行参数*/
	}

	if (i >= argc)return 1;				/*处理完了所有参数,返回成功*/
	else return 0;						/*没处理完,返回失败*/
}

/**************************************************************
* @brief	一级DEBUG输出信息
			当且仅当调试等级大于等于1时才输出
* @param	const char*	cmd	输出模式串
* @param	const char*	...	可变长参数列表
* @return	void
**************************************************************/
void debugPrintf(const char* cmd, ...) {
	/*当且仅当调试等级大于等于1时才执行*/
	if (gDebugLevel < 1) return; 
	va_list args;
	va_start(args, cmd);
	printf("%d", logNum);			/*输出序号*/
	logNum++;
	logNum = logNum % MAX_LOGNUM;	/*循环序号*/
	time_t t;
	struct tm* lt;    
	time(&t);						/*获取时间戳*/
	lt = localtime(&t);				/*转为tm结构体*/
	printf("	%d/%d/%d %d:%d:%d   	", lt->tm_year + 1900, lt->tm_mon, lt->tm_mday,
		lt->tm_hour, lt->tm_min, lt->tm_sec);/*输出日志*/
	vprintf(cmd, args);
	va_end(args);
}