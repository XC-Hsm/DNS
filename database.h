/***************************************************************
* @file		database.h
* @brief	用户请求的缓存队列、cache和文件的维护管理等操作
* @author	韩世民、左涪波、张渝鹏
* @version	1.0.0
* @date		2021-06-01
***************************************************************/
#pragma once
#include <stdio.h>
#include <winsock2.h>
#include <string.h>
#include "control.h"

#define MAX_QUERIES 250			/*最大支持的同时等待的用户query数*/
#define MAX_CACHE_SIZE 100		/*dns缓存的容量*/

typedef unsigned short DNSID; /*适用于DNS报文的ID类型*/

/**************************************************************
* @brief	存放用户query记录的结构
* @attr     SOCKADDR_IN addr	socket地址
* @attr	    DNSID originId		用户发送的DNS报文的ID
* @attr     unsigned char r  	表示这个是否被reply了
* @attr     expireTime			超时时刻
**************************************************************/
typedef struct {
	SOCKADDR_IN addr;	
	DNSID originId;
	unsigned char r;
	int expireTime;

} CRecord;

/**************************************************************
* @brief	ClientTable结构体
* @attr     CRecord base[MAX_QUERIES];	等待窗口
* @attr	    int front					头指针
* @attr     int rear					尾指针
**************************************************************/
typedef struct {
	CRecord base[MAX_QUERIES];
	int front;
	int rear;
} CQueue;

/**************************************************************
* @brief	cache单元
* @attr     domainName[MAX_DOMAINNAME]  域名
* @attr	    ip[MAX_IP_BUFSIZE]		    ip
* @attr     ttl				            ttl
**************************************************************/
typedef struct {
	char domainName[MAX_DOMAINNAME];/*域名*/
	char ip[MAX_IP_BUFSIZE];		/*ip*/
	int ttl;						/*ttl*/
} DNScache;

/**************************************************************
* @brief	初始化ClientTable
* @param	void
* @return	void
**************************************************************/
extern void InitCTable();

/**************************************************************
* @brief	打印ClientTable状态
* @param	void
* @return	void
**************************************************************/
extern void DebugCTable();

/**************************************************************
* @brief	返回ClientTable已经使用的空间
* @param	void
* @return	队列长度
**************************************************************/
int CTableUsage();

/**************************************************************
* @brief	向ClientTable的队尾添加记录
* @param	SOCKADDR* pAddr	  地址
* @param	DNSID id		  原id
* @return	int			      0/1表示是否成功
* @remark	自动计算超时时刻
**************************************************************/
extern int PushCRecord(const SOCKADDR_IN* pAddr, DNSID* pId);

/**************************************************************
* @brief	将队首记录弹出
* @param	DNSID id		指定的DNS报文
* @return	int			    0/1表示是否成功
**************************************************************/
extern int PopCRecord();

/**************************************************************
* @brief	将指定记录修改为已回复
* @param	DNSID id		指定的DNS报文
* @return	int			    0/1表示是否成功
**************************************************************/
extern int SetCRecordR(DNSID id);

/**************************************************************
* @brief	根据转发ID查找原ID和地址
* @param	DNSID id		     id
* @param    CRecord* pRecord	 存放record的地址 传出参数
* @return	int			         0/1表示是否成功
**************************************************************/
extern int FindCRecord(DNSID id, CRecord* pRecord);

/**************************************************************
* @brief	获取队尾元素的序号
* @param	void
* @return	int			序号
**************************************************************/
extern int GetCTableRearIndex();

/**************************************************************
* @brief	获取队首元素序号
* @param	void
* @return	int			序号
**************************************************************/
extern int GetCTableFrontIndex();

/**************************************************************
* @brief	从ascii文件中构建DNS记录表
* @param	void
* @return	char 0 失败
* @return	char 1 成功
**************************************************************/
extern int BuildDNSDatabase();

/**************************************************************
* @brief	在database中分级查找域名
* @param	domainName	     域名
* @param    ip			     ip地址，返回值,请保证至少
                             有16字节的空间(15个字符+\0)
* @return	0                没找到
* @return	1                找到了
**************************************************************/
extern int FindInDNSDatabase(const char* domainName, char* ip);

/**************************************************************
* @brief	计算位置
* @param	domainName	     域名
* @param    cnt			     冲突次数
* @return	int              位置
**************************************************************/
int Hash(const char* domainName, int cnt);

/**************************************************************
* @brief	添加记录到DNSCache
* @param	domainName	     域名
* @param    ip			     ip
* @param    ttl              ttl
* @param    loc              添加位置
* @return	void
**************************************************************/
extern void InsertHash(const char* domainName, const char* ip, int ttl, int loc);

/**************************************************************
* @brief	在DNSCache中查找记录
* @param	domainName	     域名
* @param    loc              位置
                             (找到位置或添加位置,取决于return)
* @return	0                未找到
* @return   1                找到了
**************************************************************/
extern int SearchHash(const char* domainName, char* ip, int* loc);

/**************************************************************
* @brief	更新DNScache里面的TTL和删除过期的DNS记录
* @param	void
* @return   void
**************************************************************/
extern void UpdateCache();

/**************************************************************
* @brief	将域名和对应的ip插入到文件
* @param	domainName	    域名
* @param    ip				ip
* @return   0				成功
**************************************************************/
extern int InsertIntoDNSTXT(const char* domainName, const char* ip);

/**************************************************************
* @brief	将cache中的过期记录删除
* @param	i				cache[i]
* @return   void
**************************************************************/
extern void DelFromDNSTXT(int i);

/**************************************************************
* @brief	能否在文件中找到域名的记录
* @param	name			待查询域名
* @param	ip				ip
* @return	0                未找到
* @return   1                找到了
**************************************************************/
extern int FindIPByDNSinTXT(const char* name, char* ip);
