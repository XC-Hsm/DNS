/***************************************************************
* @file		control.h
* @brief	命令行参数处理、日志输出、缓冲区处理函数
* @author	韩世民、左涪波、张渝鹏
* @version	1.0.0
* @date		2021-06-01
***************************************************************/
#pragma once
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

#define MAX_BUFSIZE 512				/*缓冲区大小上限*/
#define MAX_IP_BUFSIZE 16			/*IP的最大长度*/
#define MAX_DOMAINNAME 100			/*域名的最大长度*/
#define TTL 120						/*TTL的初始值*/
#define TIMEOUT 3					/*客户端请求超时时长*/
#define MAX_LOGNUM 100000			/*日志序号最大长度*/
typedef enum { dgram_arrival, nothing } event_type; /*事件类型*/
extern char gDBtxt[100];			/*存储数据库文件名*/
extern char tmpgDBtxt[100];			/*存储数据库临时文件名*/
extern int abc;
extern char addrDNSserv[16];		/*DNS服务器地址*/
int gDebugLevel;					/*调试等级*/

/**************************************************************
* @brief	以字节格式输出缓冲区内容
			当且仅当调试等级为2时才输出
* @param	const unsigned char*	buf		缓冲区指针
* @param	int						bufSize	缓冲区大小
* @return	void
**************************************************************/
extern void DebugBuffer(const unsigned char* buf, int bufSize);

/**************************************************************
* @brief	二级DEBUG输出信息
			当且仅当调试等级为2时才输出
* @param	const char*	cmd	输出模式串
* @param	const char*	...	可变长参数列表
* @return	void
**************************************************************/
extern void DebugPrintf(const char* cmd, ...);

/**************************************************************
* @brief	清空缓冲区
* @param	unsigned char*	buf		缓冲区指针
* @param	int				bufSize	缓冲区大小
* @return	void
**************************************************************/
extern void ClearBuffer(unsigned char* buf, int bufSize);

/**************************************************************
* @brief	命令行参数处理，操作模式设置
			设置调试级别
			设置DNS服务器IP地址
			设置数据库文件地址
* @param	int		argc	参数个数
* @param	char*	argv[]	参数列表
* @return	void
**************************************************************/
extern int dealOpts(int argc, char* argv[]);

/**************************************************************
* @brief	一级DEBUG输出信息
			当且仅当调试等级大于等于1时才输出
* @param	const char*	cmd	输出模式串
* @param	const char*	...	可变长参数列表
* @return	void
**************************************************************/
extern void debugPrintf(const char* cmd, ...);