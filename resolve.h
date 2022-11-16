/***************************************************************
* @file		resolve.h
* @brief	处理DNS响应和客户端请求
* @author	韩世民、左涪波、张渝鹏
* @version	1.0.0
* @date		2021-06-01
***************************************************************/
#pragma once
#include <string.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include "database.h"
#include "control.h"

/*可以使用database的所有接口,以及control中定义的宏*/

/**************************************************************
* @brief	DNS头部结构体,12字节
* @attr     每个属性两字节
**************************************************************/
typedef struct dnsheader {
	unsigned short ID;      /*ID*/
	unsigned short FLAGS;	/*2字节的各种flag*/
	unsigned short QDCOUNT; /*查询字段*/
	unsigned short ANCOUNT; /*应答字段*/
	unsigned short NSCOUNT; 
	unsigned short ARCOUNT;  
} DNSHeader;

/**************************************************************
* @brief	处理接收到query包的情况
* @param	recvBuf:	接收缓存
* @param	sendBuf:	发送缓存
* @param    recvByte:	接收到的字节数
* @param    addrCli:	&新的目标地址
* @return	int         最终写入到sendBuf的字节数
**************************************************************/
extern int ResolveQuery(const unsigned char* recvBuf, unsigned char* sendBuf, int recvByte, SOCKADDR_IN* addrCli);

/**************************************************************
* @brief	处理接收到Response包的情况
* @param	recvBuf:	接收缓存
* @param	sendBuf:	发送缓存
* @param    recvByte:	接收到的字节数
* @param    addrCli:	&新的目标地址
* @return	int         最终写入到sendBuf的字节数
**************************************************************/
extern int ResolveResponse(const unsigned char* recvBuf, unsigned char* sendBuf, int recvByte, SOCKADDR_IN* addrCli);

