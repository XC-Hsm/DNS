/***************************************************************
* @file		resolve.c
* @brief	����DNS��Ӧ�Ϳͻ�������
* @author	�������󸢲���������
* @version	1.0.0
* @date		2021-06-01
***************************************************************/
#include "resolve.h"

/**************************************************************
* @brief	�ֶ�������(n)�����ݿ�������(p)���໥ת��
			(�ο�������->�����������)
* @param	nName		�����ϵ�����
* @param	pName		�����ϵ�����
* @return	length		��������
**************************************************************/
static int domainName_ntop(const unsigned char* nName, unsigned char* pName) {
	/*��һ���ַ���Ҫ*/

	memcpy(pName, nName + 1, strlen((char*)nName));/*��������Ҳ��\0����,����ʹ��strlen*/
	int length;
	length = strlen((char*)nName) + 1;
	int i = nName[0];/*��һ�����λ��*/
	while (pName[i]) {						/*������0ʱ����*/
		int offset = pName[i];				/*��һ�εĳ���*/
		pName[i] = '.';						/*�ָ�*/
		/*printf("i=%d\n", i);*/			/*������*/
		i += offset + 1;					/*������һ���ָ���*/
	}
	return length;
}

/**************************************************************
* @brief	������յ�query�������
* @param	recvBuf:	���ջ���
* @param	sendBuf:	���ͻ���
* @param    recvByte:	���յ����ֽ���
* @param    addrCli:	&�µ�Ŀ���ַ
* @return	int         ����д�뵽sendBuf���ֽ���
**************************************************************/
extern int ResolveQuery(const unsigned char* recvBuf, unsigned char* sendBuf, int recvByte, SOCKADDR_IN* addrCli) {
	char ip[MAX_IP_BUFSIZE] = { '\0' };						/*���ip*/
	char Cliip[MAX_IP_BUFSIZE] = { '\0' };					/*��ſͻ���ip*/
	char domainName[MAX_DOMAINNAME] = { '\0' };	/*�������*/
	domainName_ntop(recvBuf + 12, domainName);	/*��ȡ����*/
	inet_ntop(AF_INET, (void*)&addrCli->sin_addr, Cliip, 16);
	
	//DebugPrintf("����Ŀ�꣺%s ����IP��%s:%d\n", domainName, Cliip, htons(addrCli->sin_port));
	if (FindInDNSDatabase(domainName, ip)) {	/*����ҵ���¼*/
		debugPrintf("Client��%s:%d	%s->%s\n", Cliip, htons(addrCli->sin_port), domainName, ip);
		memcpy(sendBuf, recvBuf, recvByte);		/*���������ݸ��Ƶ����ͻ���*/
		sendBuf[2] |= 0x80;						/*QR=response*/
		sendBuf[2] |= 0x01;						/*Authority=0*/
		sendBuf[3] |= 0x80;						/*RA=1*/

		if (!strcmp(ip, "0.0.0.0")) {			/*������Ч*/
			sendBuf[7] |= 0x00;					/*Answer count = 0*/
			sendBuf[3] |= 0x03;					/*����������*/
			return recvByte;					/*���ĳ���Ϊ�ײ�����*/
		}

		sendBuf[7] |= 0x01;						/*Answer count = 1*/
		unsigned char answer[16] = { 0 };		/*���answer����Ϊ0*/
		answer[0] = 0xc0;						/*ʹ��ָ���ʾ����*/
		answer[1] = 0x0c;						/*�����ĵ�ַΪ��12�ֽ�*/
		answer[3] = 0x01;						/*TYPE = 1*/
		answer[5] = 0x01;						/*CLASS = 1*/
		answer[9] = 0x78;						/*TTL = 120s*/
		answer[11] = 0x04;						/*DATA LENGTH = 4*/
		inet_pton(AF_INET, ip, answer + 12);	/*IP��ַ*/
		memcpy(sendBuf + recvByte, answer, 16);	/*��answer���ӵ�query����*/
		return recvByte + 16;					/*sendBuf�Ĵ�С*/
	}
	else {
		DebugPrintf("û���ڱ����ҵ���¼: %s\n", domainName);
		/*��������ݿ����Ҳ���,��ֱ��ת��������DNS������---*/
		DNSHeader* header = (DNSHeader*)recvBuf;
		DebugPrintf("�յ�IDΪ%x��������\n", ntohs(header->ID));
		DNSID newID = ntohs(header->ID);
		if (PushCRecord(addrCli, &newID)) {/*����ɹ��������*/
			//SetTime();
			/*��д���ͻ���*/
			memcpy(sendBuf, recvBuf, recvByte);
			/*����ת��ID*/
			header = (DNSHeader*)sendBuf;
			header->ID = htons(newID);
			/*����Ŀ���ַ*/
			addrCli->sin_family = AF_INET;
			addrCli->sin_port = htons(53);
			inet_pton(AF_INET, addrDNSserv, &addrCli->sin_addr);
			return recvByte;
		}
		else { /*����������ʧ��*/
			return -1;
		}
	}
}

/**************************************************************
* @brief	������յ�Response�������
* @param	recvBuf:	���ջ���
* @param	sendBuf:	���ͻ���
* @param    recvByte:	���յ����ֽ���
* @param    addrCli:	&�µ�Ŀ���ַ
* @return	int         ����д�뵽sendBuf���ֽ���
**************************************************************/
extern int ResolveResponse(const unsigned char* recvBuf, unsigned char* sendBuf, int recvByte, SOCKADDR_IN* addrCli) {
	//char temttl[16] = { '\0' };
	char ip[MAX_IP_BUFSIZE] = { '\0' };					    /*���ip*/
	char Cliip[MAX_IP_BUFSIZE] = { '\0' };				    /*��ſͻ���ip*/
	char tmpip[MAX_IP_BUFSIZE] = { '\0' };
	char domainName[MAX_DOMAINNAME] = { '\0' };			    /*�������*/
	int domainlen;
	int loc;
	domainlen = domainName_ntop(recvBuf + 12, domainName);	/*��ȡ����*/
	inet_ntop(AF_INET, recvBuf + (12 + domainlen + 16), ip, 16);
	int ttl = ntohl(*((int*)(recvBuf + (12 + domainlen + 10))));
	if (!(recvBuf[3] & 0x0f)) { /*Z�ֶ���ԶΪ0, RCODEΪ0��ʾ����*/
		if (!SearchHash(domainName, ip, &loc)) {            /*cache�в�����*/
			InsertHash(domainName, ip, ttl, loc);           /*����cache*/
			if (!FindIPByDNSinTXT(domainName, tmpip)) {     /*�ļ��в�����*/
				InsertIntoDNSTXT(domainName, ip);           /*�����ļ�*/
			}
		}
	}

	const DNSHeader* recvHeader = (DNSHeader*)recvBuf;
	DebugPrintf("�յ�IDΪ%x����Ӧ����\n", ntohs(recvHeader->ID));
	/*�ⲿDNS������ID��newID��FindCRecordǰ��¼Buf*/
	DNSID newID = ntohs(recvHeader->ID);

	CRecord record = { 0 };
	CRecord* pRecord = &record;

	if (FindCRecord((DNSID)newID, (CRecord*)pRecord) == 1) {/*�����clientTable���ҵ�newID��¼*/
		if (pRecord->r == 0) {
			SetCRecordR(newID); /*δ�ظ���ظ�����r��Ϊ1*/
			/*printf("IDΪ %hu �ı����Ѿ��ظ�\n", newID);*/
		}
		else {
			return -1; /*�ظ����Ͳ�������*/
		}
		memcpy(sendBuf, recvBuf, recvByte);			/*�������ݸ��Ƶ����ͻ���*/
		/*��newID����originID*/
		DNSHeader* sendHeader = (DNSHeader*)sendBuf;	/*header��Ϊ*/
		sendHeader->ID = htons(pRecord->originId);
		/*���͵�ַ��IPv4,��ַ���˿�:��CRecord�л�ȡ��ǰID��Ӧ��Դ��ַ���˿�*/
		*addrCli = pRecord->addr;
		inet_ntop(AF_INET, (void*)&addrCli->sin_addr, Cliip, 16);
		debugPrintf("Client��%s:%d	%s->%s\n", Cliip, htons(addrCli->sin_port), domainName, ip);
		return recvByte;
	}
	else {
		return -1;
	}
}