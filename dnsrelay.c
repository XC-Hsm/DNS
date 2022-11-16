/***************************************************************
* @file		dnsrelay.c
* @brief    DNS中继主程序
* @author	韩世民、左涪波、张渝鹏
* @version	1.0.0
* @date		2021-06-01
***************************************************************/
#include <stdio.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <conio.h>
#include <time.h>
#include "control.h"
#include "database.h"
#include "resolve.h"
#pragma comment(lib,"ws2_32.lib")

/**************************************************************
* @brief	更新cache、判断是否接收到数据包
* @param	SOCKET			fd
* @return	event_type		dgram_arrival
*							nothing
**************************************************************/
event_type WaitForEvent(SOCKET fd) {

	UpdateCache();	/*更新cache、文件*/

	while (1)
	{
		CRecord record = { 0 };
		if (CTableUsage()	//非空
			&& FindCRecord(GetCTableFrontIndex(), &record)//取队首
			&& record.r) {//判断是否发送
			PopCRecord();//出队
			continue;
		}

		if (CTableUsage()
			&& FindCRecord(GetCTableFrontIndex(), &record)
			&& record.expireTime
			&& time(NULL) > record.expireTime) {
			DebugPrintf("TimeOUT DELETE\n");
			PopCRecord();
			continue;
		}

		else
			break;
	}

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);
	struct timeval t = { 1,0 };	/*设置超时时间*/
	select(fd + 1, &readfds, 0, 0, &t);/*非阻塞*/

	if (FD_ISSET(fd, &readfds)) {	/*有事件发生*/
		return dgram_arrival;
	}

	return nothing;
}

int main(int argc, char* argv[]) {
	if (dealOpts(argc, argv)) {	/*成功获取参数*/
		debugPrintf("Name server %s:53\n", addrDNSserv);
		debugPrintf("Debug level %d\n", gDebugLevel);
		debugPrintf("Database using %s\n", gDBtxt);
	}
	else {					/*获取参数失败*/
		debugPrintf("Please use the following format:\n"
			"dnsrelay [-d|-dd] [dns-server-ipaddr] [filename]");
		return 0;
	}

	if (!BuildDNSDatabase()) {
		debugPrintf("Failed to build the database.\n");
		return 0;
	}

	WSADATA wsaData;								/*协议版本信息*/
	SOCKADDR_IN addrSrv;							/*服务端(dnsrelay)地址*/
	SOCKADDR_IN addrCli;							/*客户端地址*/
	int addrCliSize = sizeof(addrCli);				/*客户端地址的大小*/
	int Port = 53;									/*socket绑定端口*/
	char ipStrBuf[20] = { '\0' };					/*存放IP地址的栈内存*/
	event_type event;								/*接收事件*/
	unsigned char recvBuf[MAX_BUFSIZE] = { '\0' };	/*接收缓冲*/
	unsigned char sendBuf[MAX_BUFSIZE] = { '\0' };	/*发送缓冲*/
	int recvByte = 0;								/*recvBuf存放的报文大小*/
	int sendByte = 0;								/*sendBuf存放的报文大小*/

	/* 获取socket版本2.2 */
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		debugPrintf("Server: WSAStartup failed with error %ld\n", WSAGetLastError());
		return 0;
	}

	/*
		创建socket:
		地址族:	AF_INET		-- IPv4
		类型:	SOCK_DGRAM	-- udp数据报
		协议:	UDP			-- udp
	*/
	SOCKET sockSrv = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockSrv == INVALID_SOCKET) {
		debugPrintf("Invalid socket error %ld\n", WSAGetLastError());
		WSACleanup();/* clean up */
		return 0;
	}
	else debugPrintf("Socket() is OK!\n");

	/* 构造地址 */
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(Port);
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);/* 本机所有IP都可以连接 */

	/* 绑定socket */
	if (bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(addrSrv)) == SOCKET_ERROR) {
		/* 绑定错误 */
		debugPrintf("Failed to bind() with error %ld\n", WSAGetLastError());
		closesocket(sockSrv);	/* 关闭socket */
		WSACleanup();			/* clean up */
		return 0;
	}
	else debugPrintf("Bind() is OK!\n");

	InitCTable();	/*初始化clientTable为空队列*/

	/* 开始无尽的循环, 按下 Esc 退出循环 */
	while (!(_kbhit() && _getch() == 27)) {

		event = WaitForEvent(sockSrv);
		switch (event)
		{
		case dgram_arrival:

			/*清空buffer*/
			ClearBuffer(recvBuf, recvByte);
			ClearBuffer(sendBuf, sendByte);

			/*接收报文*/
			DebugPrintf("datagram arrived. Recieving...\n");
			recvByte = recvfrom(sockSrv, (char*)recvBuf, MAX_BUFSIZE, 0, (SOCKADDR*)&addrCli, &addrCliSize);
			if (recvByte <= 0) {
				DebugPrintf("***************************************************\n");
				DebugPrintf("recvfrom() failed with error %ld\n", WSAGetLastError());
				DebugPrintf("***************************************************\n");
				recvByte = 0;	/*置零*/
				break;
			}
			else
				DebugPrintf("datagram received. %d Bytes in all.\n", recvByte);

			/*判断接收的是response还是query,更新报文sendBuf,更新发送目标addrCli(不一定是客户端,也可能是DNS服务器*/
			if ((recvBuf[2] & 0x80) >> 7 == 0) {
				DebugPrintf("收到一个请求报文。内容如下:\n");
				DebugBuffer(recvBuf, recvByte);	/*打印buffer--debug*/
				sendByte = ResolveQuery(recvBuf, sendBuf, recvByte, &addrCli);
				
				if (sendByte < 0) {	/*处理query失败*/
					DebugPrintf("***************************************************\n");
					DebugPrintf("failed to solved query.\n");
					DebugPrintf("***************************************************\n");
					break;
				}
			}
			else {
				DebugPrintf("收到一个响应报文。内容如下:\n");
				DebugBuffer(recvBuf, recvByte);	/*打印buffer--debug*/
				sendByte = ResolveResponse(recvBuf, sendBuf, recvByte, &addrCli);
				if (sendByte < 0) {	/*处理response失败*/
					DebugPrintf("***************************************************\n");
					DebugPrintf("failed to solved response.\n");
					DebugPrintf("***************************************************\n");
					break;
				}
			}

			/*发送报文*/
			DebugPrintf("datagram sending to %s:%d\n",
				inet_ntop(AF_INET, (void*)&addrCli.sin_addr, ipStrBuf, 16),
				htons(addrCli.sin_port)
			);
			addrCliSize = sizeof(addrCli);
			sendto(sockSrv, (char*)sendBuf, sendByte, 0, (SOCKADDR*)&addrCli, addrCliSize);
			DebugPrintf("datagram sending succeed %d Bytes in all.\n", sendByte);

			/*打印发送的内容--debug*/
			if ((sendBuf[2] & 0x80) >> 7 == 0) {
				DebugPrintf("发送一个请求报文。内容如下:\n");
				DebugBuffer(sendBuf, sendByte);/*打印buffer*/
			}
			else {
				DebugPrintf("发送一个响应报文。内容如下:\n");
				DebugBuffer(sendBuf, sendByte);/*打印buffer*/
			}
			break;

		default:
			break;
		}
	}

	/* 运行结束, socket关闭 */
	if (closesocket(sockSrv) != 0)
		DebugPrintf("close socket failed with error %ld\n", GetLastError());
	else
		DebugPrintf("socket closed.\n");

	/* 运行结束, 清理缓存 */
	if (WSACleanup() != 0)
		DebugPrintf("WSA clean up failed with error %ld\n", GetLastError());
	else
		DebugPrintf("clean up is OK.\n");

	return 0;
}
