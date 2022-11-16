/***************************************************************
* @file		database.c
* @brief    �û�����Ļ�����С�cache���ļ���ά������Ȳ���
* @author	�������󸢲���������
* @version	1.0.0
* @date		2021-06-01
***************************************************************/
#ifndef	_CRT_SECURE_NO_WARNINGS 
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "database.h"

static CQueue clientTable; 

/**************************************************************
* @brief	��ʼ��ClientTable
* @param	void
* @return	void
**************************************************************/
void InitCTable() {
	clientTable.front = 0;
	clientTable.rear = 0;
	DebugPrintf("ClientTable init.\n");
}

/**************************************************************
* @brief	����ClientTable�Ѿ�ʹ�õĿռ�
* @param	void
* @return	���г���
**************************************************************/
int CTableUsage() {
	int used = clientTable.rear - clientTable.front;
	used = used < 0 ? used + MAX_QUERIES : used;
	return used;
}

/**************************************************************
* @brief	��ClientTable�Ķ�β��Ӽ�¼
* @param	SOCKADDR* pAddr	  ��ַ
* @param	DNSID id		  ԭid
* @return	int			      0/1��ʾ�Ƿ�ɹ�
* @remark	�Զ����㳬ʱʱ��
**************************************************************/
int PushCRecord(const SOCKADDR_IN* pAddr, DNSID* pId) {
	if ((clientTable.rear + 1) % MAX_QUERIES == clientTable.front) {
		DebugPrintf("��������,��������\n.");
		return 0;
	}
	else {
		clientTable.base[clientTable.rear].addr = *pAddr;
		clientTable.base[clientTable.rear].originId = *pId;
		clientTable.base[clientTable.rear].r = 0;
		/*����ȥ��ʱ����㳬ʱʱ��*/
		clientTable.base[clientTable.rear].expireTime = (int)time(NULL) + TIMEOUT;
		*pId = clientTable.rear;/*��ȡ�µ�ID*/
		clientTable.rear = (clientTable.rear + 1) % MAX_QUERIES;
		return 1;
	}
}

/**************************************************************
* @brief	�����׼�¼����
* @param	DNSID id		ָ����DNS����
* @return	int			    0/1��ʾ�Ƿ�ɹ�
**************************************************************/
int PopCRecord() {
	if (clientTable.rear == clientTable.front) {
		DebugPrintf("����Ϊ�գ�PopCRecord()ʧ�ܡ�\n");
		return 0;
	}
	else {
		clientTable.front = (clientTable.front + 1) % MAX_QUERIES;
		return 1;
	}
}

/**************************************************************
* @brief	��ָ����¼�޸�Ϊ�ѻظ�
* @param	DNSID id		ָ����DNS����
* @return	int			    0/1��ʾ�Ƿ�ɹ�
**************************************************************/
int SetCRecordR(DNSID id) {
	if ((
		clientTable.front <= clientTable.rear
		&& (id < clientTable.rear && id >= clientTable.front)
		) || (
			clientTable.front > clientTable.rear
			&& (id < clientTable.rear || id >= clientTable.front))
		) {
		clientTable.base[id].r = 1;
		return 1;
	}
	else {
		DebugPrintf("***************************************************\n");
		DebugPrintf("ERROR	Ҫ�޸ĵļ�¼������,����Խ��,SetCRecordR()ʧ�� ID=%d\n", id);
		DebugPrintf("***************************************************\n");
		return 0;
	}
}

/**************************************************************
* @brief	����ת��ID����ԭID�͵�ַ
* @param	DNSID id		     id
* @param    CRecord* pRecord	 ���record�ĵ�ַ ��������
* @return	int			         0/1��ʾ�Ƿ�ɹ�
**************************************************************/
int FindCRecord(DNSID id, CRecord* pRecord) {
	if ((
		clientTable.front <= clientTable.rear
		&& (id < clientTable.rear && id >= clientTable.front)
		) || (
			clientTable.front > clientTable.rear
			&& (id < clientTable.rear || id >= clientTable.front))
		) {
		*pRecord = clientTable.base[id];
		DebugPrintf("FindCRecord	DNSID=%d	EPT=%d	CLIID=%d\n", id, clientTable.base[id].expireTime, clientTable.base[id].originId);
		return 1;
	}
	else {
		DebugPrintf("***************************************************\n");
		DebugPrintf("ERROR	Ҫ���ҵļ�¼������,����Խ��,SetCRecordR()ʧ��\n");
		DebugPrintf("***************************************************\n");
		return 0;
	}
}

/**************************************************************
* @brief	��ȡ��βԪ�ص����
* @param	void
* @return	int			���
**************************************************************/
int GetCTableRearIndex() {
	/*����rear-1*/
	return ((clientTable.rear + MAX_QUERIES - 1) % MAX_QUERIES);
}

/**************************************************************
* @brief	��ȡ����Ԫ�����
* @param	void
* @return	int			���
**************************************************************/
int GetCTableFrontIndex() {
	return clientTable.front;
}

static FILE* dbTXT = NULL;		/*�ı����ݿ����*/
static fpos_t dbHome = 0;		/*����ÿ�����´��ļ�, */
/*cache����, ��λ�þͲ�, ����ô���ʵ��һ�°�, ���Ч��cache*/
static DNScache cache[MAX_CACHE_SIZE] = { 0 };

/**************************************************************
* @brief	�ܷ����ļ����ҵ������ļ�¼
* @param	name			����ѯ����
* @param	ip				ip
* @return	0                δ�ҵ�
* @return   1                �ҵ���
**************************************************************/
int FindIPByDNSinTXT(const char* name, char* ip) {
	char retName[MAX_DOMAINNAME] = { '\0' };
	fsetpos(dbTXT, &dbHome);	/*���õ���ʼλ��*/
	while (!feof(dbTXT)) {
		if(fscanf(dbTXT, "%s %s", ip, retName) == 2) {
			if (!strcmp(retName, name)) {
				DebugPrintf("�ļ����У�\n");
				return 1;
			}
		}
		
	}
	if (feof(dbTXT)) {
		DebugPrintf("�ļ�δ���У�\n");
		return 0;	/*�ļ�����,δ�ҵ�*/
	}
	return 0;
}

/**************************************************************
* @brief	�������Ͷ�Ӧ��ip���뵽�ļ�
* @param	domainName	    ����
* @param    ip				ip
* @return   0				�ɹ�
**************************************************************/
int InsertIntoDNSTXT(const char* domainName, const char* ip) {
	fseek(dbTXT, 0L, SEEK_END);
	DebugPrintf("FileSAVE: %s, %s\n", domainName, ip);
	fprintf(dbTXT, "%s %s\n", ip, domainName);
	return 0;
}

/**************************************************************
* @brief	��cache�еĹ��ڼ�¼ɾ��
* @param	i				cache[i]
* @return   void
**************************************************************/
void DelFromDNSTXT(int i) {
	char retName[MAX_DOMAINNAME] = { '\0' };
	char retIP[MAX_IP_BUFSIZE] = { '\0' };
	long tmp = 0;
	int ch = 0;
	fsetpos(dbTXT, &dbHome);
	FILE* fp = fopen(tmpgDBtxt, "w+");//��д�½�һ����ʱ�ļ�
	while (!feof(dbTXT) && fscanf(dbTXT, "%s %s", retIP, retName)==2)//��ԭ�ļ���һ�����
	{
		if (strcmp(retName, cache[i].domainName) || strcmp(retIP, cache[i].ip))//����Ҫɾ��������
		{
			fprintf(fp, "%s %s\n", retIP, retName);
		}
	}
	fseek(fp, 0, SEEK_SET);
	fclose(dbTXT);
	dbTXT=fopen(gDBtxt, "w+");
	while (!feof(fp) && fscanf(fp, "%s %s", retIP, retName)==2)//��ԭ�ļ���һ�����
	{
		fprintf(dbTXT, "%s %s\n", retIP, retName);
	}
	fclose(fp);
	fclose(dbTXT);
	dbTXT = fopen(gDBtxt, "a+");
	fgetpos(dbTXT, &dbHome);
	DebugPrintf("FILEDelete %s", cache[i].domainName);
}

static time_t cacheLastCheckTime = 0; /*��һ�μ���ʱ��, ��ʼ��Ϊ0, �����ʱ��ż��*/
static int FileUpdateTimer = 0;/*�����ļ�������, ��ʼ��Ϊ0, ÿ���һ�������ʱ�����ļ�*/

/**************************************************************
* @brief	����λ��
* @param	domainName	     ����
* @param    cnt			     ��ͻ����
* @return	int              λ��
**************************************************************/
int Hash(const char* domainName, int cnt) {
	char* tmp;
	int sum = 0;
	tmp = (char*)domainName;
	while (*tmp != '\0') {
		sum += (*tmp);
		tmp++;
	}
	return (sum + cnt) % MAX_CACHE_SIZE;
}

/**************************************************************
* @brief	��DNSCache�в��Ҽ�¼
* @param	domainName	     ����
* @param    loc              λ��
							 (�ҵ�λ�û����λ��,ȡ����return)
* @return	0                δ�ҵ�
* @return   1                �ҵ���
**************************************************************/
int SearchHash(const char* domainName, char* ip, int* loc) {
	int cnt = 0;
	*loc = Hash(domainName, cnt);
	while (cnt < MAX_CACHE_SIZE / 2 && cache[*loc].ttl > 0 && strcmp(cache[*loc].domainName, domainName))
	{
		cnt++;
		*loc = Hash(domainName, cnt);
	}
	if (!strcmp(cache[*loc].domainName, domainName)) {
		sprintf(ip, "%s", cache[*loc].ip);
		return 1;
	}	
	else {
		return 0;
	}
}

/**************************************************************
* @brief	��Ӽ�¼��DNSCache
* @param	domainName	     ����
* @param    ip			     ip
* @param    ttl              ttl
* @param    loc              ���λ��
* @return	void
**************************************************************/
void InsertHash(const char* domainName, const char* ip, int ttl, int loc) {
	sprintf(cache[loc].domainName, "%s", domainName);
	sprintf(cache[loc].ip, "%s", ip);
	cache[loc].ttl = ttl;
}

/**************************************************************
* @brief	����DNScache�����TTL��ɾ�����ڵ�DNS��¼
* @param	void
* @return   void
**************************************************************/
void UpdateCache() {
	time_t newTime = time(0);
	time_t diff = newTime - cacheLastCheckTime;
	if (diff) { //ʱ�����
		FileUpdateTimer++;
		cacheLastCheckTime = newTime;
		for (int i = 0; i < MAX_CACHE_SIZE; i++) {
			if (cache[i].ttl > 0)
			{
				cache[i].ttl -= (int)diff;
				//printf("%s : %s  TTL= %d\n", cache[i].domainName, cache[i].ip, cache[i].ttl);
				if (FileUpdateTimer == 10 && cache[i].ttl <= 0) {
					DelFromDNSTXT(i);
				}
			}
		}

	}
	if (FileUpdateTimer == 10) {
		FileUpdateTimer = 0;
	}
}

/**************************************************************
* @brief	��ascii�ļ��й���DNS��¼��
* @param	void
* @return	char 0 ʧ��
* @return	char 1 �ɹ�
**************************************************************/
int BuildDNSDatabase()
{
	/*���ı��ļ�*/
	dbTXT = fopen(gDBtxt, "a+");
	fgetpos(dbTXT, &dbHome);
	if (!dbTXT)return 0;
	return 1;
}

/**************************************************************
* @brief	��database�зּ���������
* @param	domainName	     ����
* @param    ip			     ip��ַ������ֵ,�뱣֤����
							 ��16�ֽڵĿռ�(15���ַ�+\0)
* @return	0                û�ҵ�
* @return	1                �ҵ���
**************************************************************/
int FindInDNSDatabase(const char* domainName, char* ip) {
	/*����cache������*/
	int loc;
	if (SearchHash(domainName, ip, &loc)) {
		DebugPrintf("cache����!\n");
		return 1;
	}
	if (dbTXT && FindIPByDNSinTXT(domainName, ip)) {
		/*��¼���뵽cache��*/
		InsertHash(domainName, ip, TTL, loc);
		return 1; /*db�������ҵ��˶�Ӧ��¼*/
	}
	else {
		return 0;
	}
}