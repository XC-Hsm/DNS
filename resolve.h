/***************************************************************
* @file		resolve.h
* @brief	����DNS��Ӧ�Ϳͻ�������
* @author	�������󸢲���������
* @version	1.0.0
* @date		2021-06-01
***************************************************************/
#pragma once
#include <string.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include "database.h"
#include "control.h"

/*����ʹ��database�����нӿ�,�Լ�control�ж���ĺ�*/

/**************************************************************
* @brief	DNSͷ���ṹ��,12�ֽ�
* @attr     ÿ���������ֽ�
**************************************************************/
typedef struct dnsheader {
	unsigned short ID;      /*ID*/
	unsigned short FLAGS;	/*2�ֽڵĸ���flag*/
	unsigned short QDCOUNT; /*��ѯ�ֶ�*/
	unsigned short ANCOUNT; /*Ӧ���ֶ�*/
	unsigned short NSCOUNT; 
	unsigned short ARCOUNT;  
} DNSHeader;

/**************************************************************
* @brief	������յ�query�������
* @param	recvBuf:	���ջ���
* @param	sendBuf:	���ͻ���
* @param    recvByte:	���յ����ֽ���
* @param    addrCli:	&�µ�Ŀ���ַ
* @return	int         ����д�뵽sendBuf���ֽ���
**************************************************************/
extern int ResolveQuery(const unsigned char* recvBuf, unsigned char* sendBuf, int recvByte, SOCKADDR_IN* addrCli);

/**************************************************************
* @brief	������յ�Response�������
* @param	recvBuf:	���ջ���
* @param	sendBuf:	���ͻ���
* @param    recvByte:	���յ����ֽ���
* @param    addrCli:	&�µ�Ŀ���ַ
* @return	int         ����д�뵽sendBuf���ֽ���
**************************************************************/
extern int ResolveResponse(const unsigned char* recvBuf, unsigned char* sendBuf, int recvByte, SOCKADDR_IN* addrCli);

