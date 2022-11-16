/***************************************************************
* @file		database.c
* @brief    用户请求的缓存队列、cache和文件的维护管理等操作
* @author	韩世民、左涪波、张渝鹏
* @version	1.0.0
* @date		2021-06-01
***************************************************************/
#ifndef	_CRT_SECURE_NO_WARNINGS 
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "database.h"

static CQueue clientTable; 

/**************************************************************
* @brief	初始化ClientTable
* @param	void
* @return	void
**************************************************************/
void InitCTable() {
	clientTable.front = 0;
	clientTable.rear = 0;
	DebugPrintf("ClientTable init.\n");
}

/**************************************************************
* @brief	返回ClientTable已经使用的空间
* @param	void
* @return	队列长度
**************************************************************/
int CTableUsage() {
	int used = clientTable.rear - clientTable.front;
	used = used < 0 ? used + MAX_QUERIES : used;
	return used;
}

/**************************************************************
* @brief	向ClientTable的队尾添加记录
* @param	SOCKADDR* pAddr	  地址
* @param	DNSID id		  原id
* @return	int			      0/1表示是否成功
* @remark	自动计算超时时刻
**************************************************************/
int PushCRecord(const SOCKADDR_IN* pAddr, DNSID* pId) {
	if ((clientTable.rear + 1) % MAX_QUERIES == clientTable.front) {
		DebugPrintf("队列已满,丢弃报文\n.");
		return 0;
	}
	else {
		clientTable.base[clientTable.rear].addr = *pAddr;
		clientTable.base[clientTable.rear].originId = *pId;
		clientTable.base[clientTable.rear].r = 0;
		/*塞进去的时候计算超时时间*/
		clientTable.base[clientTable.rear].expireTime = (int)time(NULL) + TIMEOUT;
		*pId = clientTable.rear;/*获取新的ID*/
		clientTable.rear = (clientTable.rear + 1) % MAX_QUERIES;
		return 1;
	}
}

/**************************************************************
* @brief	将队首记录弹出
* @param	DNSID id		指定的DNS报文
* @return	int			    0/1表示是否成功
**************************************************************/
int PopCRecord() {
	if (clientTable.rear == clientTable.front) {
		DebugPrintf("队列为空，PopCRecord()失败。\n");
		return 0;
	}
	else {
		clientTable.front = (clientTable.front + 1) % MAX_QUERIES;
		return 1;
	}
}

/**************************************************************
* @brief	将指定记录修改为已回复
* @param	DNSID id		指定的DNS报文
* @return	int			    0/1表示是否成功
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
		DebugPrintf("ERROR	要修改的记录不存在,索引越界,SetCRecordR()失败 ID=%d\n", id);
		DebugPrintf("***************************************************\n");
		return 0;
	}
}

/**************************************************************
* @brief	根据转发ID查找原ID和地址
* @param	DNSID id		     id
* @param    CRecord* pRecord	 存放record的地址 传出参数
* @return	int			         0/1表示是否成功
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
		DebugPrintf("ERROR	要查找的记录不存在,索引越界,SetCRecordR()失败\n");
		DebugPrintf("***************************************************\n");
		return 0;
	}
}

/**************************************************************
* @brief	获取队尾元素的序号
* @param	void
* @return	int			序号
**************************************************************/
int GetCTableRearIndex() {
	/*返回rear-1*/
	return ((clientTable.rear + MAX_QUERIES - 1) % MAX_QUERIES);
}

/**************************************************************
* @brief	获取队首元素序号
* @param	void
* @return	int			序号
**************************************************************/
int GetCTableFrontIndex() {
	return clientTable.front;
}

static FILE* dbTXT = NULL;		/*文本数据库对象*/
static fpos_t dbHome = 0;		/*避免每次重新打开文件, */
/*cache数组, 有位置就插, 就这么随便实现一下吧, 最低效的cache*/
static DNScache cache[MAX_CACHE_SIZE] = { 0 };

/**************************************************************
* @brief	能否在文件中找到域名的记录
* @param	name			待查询域名
* @param	ip				ip
* @return	0                未找到
* @return   1                找到了
**************************************************************/
int FindIPByDNSinTXT(const char* name, char* ip) {
	char retName[MAX_DOMAINNAME] = { '\0' };
	fsetpos(dbTXT, &dbHome);	/*设置到起始位置*/
	while (!feof(dbTXT)) {
		if(fscanf(dbTXT, "%s %s", ip, retName) == 2) {
			if (!strcmp(retName, name)) {
				DebugPrintf("文件命中！\n");
				return 1;
			}
		}
		
	}
	if (feof(dbTXT)) {
		DebugPrintf("文件未命中！\n");
		return 0;	/*文件结束,未找到*/
	}
	return 0;
}

/**************************************************************
* @brief	将域名和对应的ip插入到文件
* @param	domainName	    域名
* @param    ip				ip
* @return   0				成功
**************************************************************/
int InsertIntoDNSTXT(const char* domainName, const char* ip) {
	fseek(dbTXT, 0L, SEEK_END);
	DebugPrintf("FileSAVE: %s, %s\n", domainName, ip);
	fprintf(dbTXT, "%s %s\n", ip, domainName);
	return 0;
}

/**************************************************************
* @brief	将cache中的过期记录删除
* @param	i				cache[i]
* @return   void
**************************************************************/
void DelFromDNSTXT(int i) {
	char retName[MAX_DOMAINNAME] = { '\0' };
	char retIP[MAX_IP_BUFSIZE] = { '\0' };
	long tmp = 0;
	int ch = 0;
	fsetpos(dbTXT, &dbHome);
	FILE* fp = fopen(tmpgDBtxt, "w+");//读写新建一个临时文件
	while (!feof(dbTXT) && fscanf(dbTXT, "%s %s", retIP, retName)==2)//从原文件读一个结点
	{
		if (strcmp(retName, cache[i].domainName) || strcmp(retIP, cache[i].ip))//不是要删除的内容
		{
			fprintf(fp, "%s %s\n", retIP, retName);
		}
	}
	fseek(fp, 0, SEEK_SET);
	fclose(dbTXT);
	dbTXT=fopen(gDBtxt, "w+");
	while (!feof(fp) && fscanf(fp, "%s %s", retIP, retName)==2)//从原文件读一个结点
	{
		fprintf(dbTXT, "%s %s\n", retIP, retName);
	}
	fclose(fp);
	fclose(dbTXT);
	dbTXT = fopen(gDBtxt, "a+");
	fgetpos(dbTXT, &dbHome);
	DebugPrintf("FILEDelete %s", cache[i].domainName);
}

static time_t cacheLastCheckTime = 0; /*上一次检查的时间, 初始化为0, 非零的时候才检查*/
static int FileUpdateTimer = 0;/*更新文件计数器, 初始化为0, 每秒加一，五分钟时更新文件*/

/**************************************************************
* @brief	计算位置
* @param	domainName	     域名
* @param    cnt			     冲突次数
* @return	int              位置
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
* @brief	在DNSCache中查找记录
* @param	domainName	     域名
* @param    loc              位置
							 (找到位置或添加位置,取决于return)
* @return	0                未找到
* @return   1                找到了
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
* @brief	添加记录到DNSCache
* @param	domainName	     域名
* @param    ip			     ip
* @param    ttl              ttl
* @param    loc              添加位置
* @return	void
**************************************************************/
void InsertHash(const char* domainName, const char* ip, int ttl, int loc) {
	sprintf(cache[loc].domainName, "%s", domainName);
	sprintf(cache[loc].ip, "%s", ip);
	cache[loc].ttl = ttl;
}

/**************************************************************
* @brief	更新DNScache里面的TTL和删除过期的DNS记录
* @param	void
* @return   void
**************************************************************/
void UpdateCache() {
	time_t newTime = time(0);
	time_t diff = newTime - cacheLastCheckTime;
	if (diff) { //时间变了
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
* @brief	从ascii文件中构建DNS记录表
* @param	void
* @return	char 0 失败
* @return	char 1 成功
**************************************************************/
int BuildDNSDatabase()
{
	/*打开文本文件*/
	dbTXT = fopen(gDBtxt, "a+");
	fgetpos(dbTXT, &dbHome);
	if (!dbTXT)return 0;
	return 1;
}

/**************************************************************
* @brief	在database中分级查找域名
* @param	domainName	     域名
* @param    ip			     ip地址，返回值,请保证至少
							 有16字节的空间(15个字符+\0)
* @return	0                没找到
* @return	1                找到了
**************************************************************/
int FindInDNSDatabase(const char* domainName, char* ip) {
	/*先在cache里面找*/
	int loc;
	if (SearchHash(domainName, ip, &loc)) {
		DebugPrintf("cache命中!\n");
		return 1;
	}
	if (dbTXT && FindIPByDNSinTXT(domainName, ip)) {
		/*记录插入到cache中*/
		InsertHash(domainName, ip, TTL, loc);
		return 1; /*db存在且找到了对应记录*/
	}
	else {
		return 0;
	}
}