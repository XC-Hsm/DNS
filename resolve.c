/***************************************************************
* @file		resolve.c
* @brief	处理DNS响应和客户端请求
* @author	韩世民、左涪波、张渝鹏
* @version	1.0.0
* @date		2021-06-01
***************************************************************/
#include "resolve.h"

/**************************************************************
* @brief	字段中域名(n)与数据库中域名(p)的相互转换
			(参考网络序->主机序的命名)
* @param	nName		网络上的域名
* @param	pName		主机上的域名
* @return	length		域名长度
**************************************************************/
static int domainName_ntop(const unsigned char* nName, unsigned char* pName) {
	/*第一个字符不要*/

	memcpy(pName, nName + 1, strlen((char*)nName));/*网络域名也以\0结束,可以使用strlen*/
	int length;
	length = strlen((char*)nName) + 1;
	int i = nName[0];/*第一个点的位置*/
	while (pName[i]) {						/*遍历到0时结束*/
		int offset = pName[i];				/*下一段的长度*/
		pName[i] = '.';						/*分隔*/
		/*printf("i=%d\n", i);*/			/*调试用*/
		i += offset + 1;					/*跳到下一个分隔处*/
	}
	return length;
}

/**************************************************************
* @brief	处理接收到query包的情况
* @param	recvBuf:	接收缓存
* @param	sendBuf:	发送缓存
* @param    recvByte:	接收到的字节数
* @param    addrCli:	&新的目标地址
* @return	int         最终写入到sendBuf的字节数
**************************************************************/
extern int ResolveQuery(const unsigned char* recvBuf, unsigned char* sendBuf, int recvByte, SOCKADDR_IN* addrCli) {
	char ip[MAX_IP_BUFSIZE] = { '\0' };						/*存放ip*/
	char Cliip[MAX_IP_BUFSIZE] = { '\0' };					/*存放客户端ip*/
	char domainName[MAX_DOMAINNAME] = { '\0' };	/*存放域名*/
	domainName_ntop(recvBuf + 12, domainName);	/*获取域名*/
	inet_ntop(AF_INET, (void*)&addrCli->sin_addr, Cliip, 16);
	
	//DebugPrintf("请求目标：%s 请求IP：%s:%d\n", domainName, Cliip, htons(addrCli->sin_port));
	if (FindInDNSDatabase(domainName, ip)) {	/*如果找到记录*/
		debugPrintf("Client：%s:%d	%s->%s\n", Cliip, htons(addrCli->sin_port), domainName, ip);
		memcpy(sendBuf, recvBuf, recvByte);		/*将接收内容复制到发送缓冲*/
		sendBuf[2] |= 0x80;						/*QR=response*/
		sendBuf[2] |= 0x01;						/*Authority=0*/
		sendBuf[3] |= 0x80;						/*RA=1*/

		if (!strcmp(ip, "0.0.0.0")) {			/*域名无效*/
			sendBuf[7] |= 0x00;					/*Answer count = 0*/
			sendBuf[3] |= 0x03;					/*域名不存在*/
			return recvByte;					/*报文长度为首部长度*/
		}

		sendBuf[7] |= 0x01;						/*Answer count = 1*/
		unsigned char answer[16] = { 0 };		/*填充answer区域为0*/
		answer[0] = 0xc0;						/*使用指针表示域名*/
		answer[1] = 0x0c;						/*域名的地址为第12字节*/
		answer[3] = 0x01;						/*TYPE = 1*/
		answer[5] = 0x01;						/*CLASS = 1*/
		answer[9] = 0x78;						/*TTL = 120s*/
		answer[11] = 0x04;						/*DATA LENGTH = 4*/
		inet_pton(AF_INET, ip, answer + 12);	/*IP地址*/
		memcpy(sendBuf + recvByte, answer, 16);	/*将answer附加到query后面*/
		return recvByte + 16;					/*sendBuf的大小*/
	}
	else {
		DebugPrintf("没有在本地找到记录: %s\n", domainName);
		/*如果在数据库中找不到,则直接转发给本地DNS服务器---*/
		DNSHeader* header = (DNSHeader*)recvBuf;
		DebugPrintf("收到ID为%x的请求报文\n", ntohs(header->ID));
		DNSID newID = ntohs(header->ID);
		if (PushCRecord(addrCli, &newID)) {/*如果成功加入队列*/
			//SetTime();
			/*填写发送缓冲*/
			memcpy(sendBuf, recvBuf, recvByte);
			/*更新转发ID*/
			header = (DNSHeader*)sendBuf;
			header->ID = htons(newID);
			/*更新目标地址*/
			addrCli->sin_family = AF_INET;
			addrCli->sin_port = htons(53);
			inet_pton(AF_INET, addrDNSserv, &addrCli->sin_addr);
			return recvByte;
		}
		else { /*队列已满，失败*/
			return -1;
		}
	}
}

/**************************************************************
* @brief	处理接收到Response包的情况
* @param	recvBuf:	接收缓存
* @param	sendBuf:	发送缓存
* @param    recvByte:	接收到的字节数
* @param    addrCli:	&新的目标地址
* @return	int         最终写入到sendBuf的字节数
**************************************************************/
extern int ResolveResponse(const unsigned char* recvBuf, unsigned char* sendBuf, int recvByte, SOCKADDR_IN* addrCli) {
	//char temttl[16] = { '\0' };
	char ip[MAX_IP_BUFSIZE] = { '\0' };					    /*存放ip*/
	char Cliip[MAX_IP_BUFSIZE] = { '\0' };				    /*存放客户端ip*/
	char tmpip[MAX_IP_BUFSIZE] = { '\0' };
	char domainName[MAX_DOMAINNAME] = { '\0' };			    /*存放域名*/
	int domainlen;
	int loc;
	domainlen = domainName_ntop(recvBuf + 12, domainName);	/*获取域名*/
	inet_ntop(AF_INET, recvBuf + (12 + domainlen + 16), ip, 16);
	int ttl = ntohl(*((int*)(recvBuf + (12 + domainlen + 10))));
	if (!(recvBuf[3] & 0x0f)) { /*Z字段永远为0, RCODE为0表示正常*/
		if (!SearchHash(domainName, ip, &loc)) {            /*cache中不存在*/
			InsertHash(domainName, ip, ttl, loc);           /*更新cache*/
			if (!FindIPByDNSinTXT(domainName, tmpip)) {     /*文件中不存在*/
				InsertIntoDNSTXT(domainName, ip);           /*更新文件*/
			}
		}
	}

	const DNSHeader* recvHeader = (DNSHeader*)recvBuf;
	DebugPrintf("收到ID为%x的响应报文\n", ntohs(recvHeader->ID));
	/*外部DNS给出的ID是newID，FindCRecord前记录Buf*/
	DNSID newID = ntohs(recvHeader->ID);

	CRecord record = { 0 };
	CRecord* pRecord = &record;

	if (FindCRecord((DNSID)newID, (CRecord*)pRecord) == 1) {/*如果在clientTable中找到newID记录*/
		if (pRecord->r == 0) {
			SetCRecordR(newID); /*未回复则回复并把r置为1*/
			/*printf("ID为 %hu 的报文已经回复\n", newID);*/
		}
		else {
			return -1; /*回复过就不做操作*/
		}
		memcpy(sendBuf, recvBuf, recvByte);			/*接受内容复制到发送缓存*/
		/*将newID换成originID*/
		DNSHeader* sendHeader = (DNSHeader*)sendBuf;	/*header改为*/
		sendHeader->ID = htons(pRecord->originId);
		/*发送地址族IPv4,地址及端口:从CRecord中获取当前ID对应的源地址及端口*/
		*addrCli = pRecord->addr;
		inet_ntop(AF_INET, (void*)&addrCli->sin_addr, Cliip, 16);
		debugPrintf("Client：%s:%d	%s->%s\n", Cliip, htons(addrCli->sin_port), domainName, ip);
		return recvByte;
	}
	else {
		return -1;
	}
}