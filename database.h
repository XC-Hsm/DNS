/***************************************************************
* @file		database.h
* @brief	�û�����Ļ�����С�cache���ļ���ά������Ȳ���
* @author	�������󸢲���������
* @version	1.0.0
* @date		2021-06-01
***************************************************************/
#pragma once
#include <stdio.h>
#include <winsock2.h>
#include <string.h>
#include "control.h"

#define MAX_QUERIES 250			/*���֧�ֵ�ͬʱ�ȴ����û�query��*/
#define MAX_CACHE_SIZE 100		/*dns���������*/

typedef unsigned short DNSID; /*������DNS���ĵ�ID����*/

/**************************************************************
* @brief	����û�query��¼�Ľṹ
* @attr     SOCKADDR_IN addr	socket��ַ
* @attr	    DNSID originId		�û����͵�DNS���ĵ�ID
* @attr     unsigned char r  	��ʾ����Ƿ�reply��
* @attr     expireTime			��ʱʱ��
**************************************************************/
typedef struct {
	SOCKADDR_IN addr;	
	DNSID originId;
	unsigned char r;
	int expireTime;

} CRecord;

/**************************************************************
* @brief	ClientTable�ṹ��
* @attr     CRecord base[MAX_QUERIES];	�ȴ�����
* @attr	    int front					ͷָ��
* @attr     int rear					βָ��
**************************************************************/
typedef struct {
	CRecord base[MAX_QUERIES];
	int front;
	int rear;
} CQueue;

/**************************************************************
* @brief	cache��Ԫ
* @attr     domainName[MAX_DOMAINNAME]  ����
* @attr	    ip[MAX_IP_BUFSIZE]		    ip
* @attr     ttl				            ttl
**************************************************************/
typedef struct {
	char domainName[MAX_DOMAINNAME];/*����*/
	char ip[MAX_IP_BUFSIZE];		/*ip*/
	int ttl;						/*ttl*/
} DNScache;

/**************************************************************
* @brief	��ʼ��ClientTable
* @param	void
* @return	void
**************************************************************/
extern void InitCTable();

/**************************************************************
* @brief	��ӡClientTable״̬
* @param	void
* @return	void
**************************************************************/
extern void DebugCTable();

/**************************************************************
* @brief	����ClientTable�Ѿ�ʹ�õĿռ�
* @param	void
* @return	���г���
**************************************************************/
int CTableUsage();

/**************************************************************
* @brief	��ClientTable�Ķ�β��Ӽ�¼
* @param	SOCKADDR* pAddr	  ��ַ
* @param	DNSID id		  ԭid
* @return	int			      0/1��ʾ�Ƿ�ɹ�
* @remark	�Զ����㳬ʱʱ��
**************************************************************/
extern int PushCRecord(const SOCKADDR_IN* pAddr, DNSID* pId);

/**************************************************************
* @brief	�����׼�¼����
* @param	DNSID id		ָ����DNS����
* @return	int			    0/1��ʾ�Ƿ�ɹ�
**************************************************************/
extern int PopCRecord();

/**************************************************************
* @brief	��ָ����¼�޸�Ϊ�ѻظ�
* @param	DNSID id		ָ����DNS����
* @return	int			    0/1��ʾ�Ƿ�ɹ�
**************************************************************/
extern int SetCRecordR(DNSID id);

/**************************************************************
* @brief	����ת��ID����ԭID�͵�ַ
* @param	DNSID id		     id
* @param    CRecord* pRecord	 ���record�ĵ�ַ ��������
* @return	int			         0/1��ʾ�Ƿ�ɹ�
**************************************************************/
extern int FindCRecord(DNSID id, CRecord* pRecord);

/**************************************************************
* @brief	��ȡ��βԪ�ص����
* @param	void
* @return	int			���
**************************************************************/
extern int GetCTableRearIndex();

/**************************************************************
* @brief	��ȡ����Ԫ�����
* @param	void
* @return	int			���
**************************************************************/
extern int GetCTableFrontIndex();

/**************************************************************
* @brief	��ascii�ļ��й���DNS��¼��
* @param	void
* @return	char 0 ʧ��
* @return	char 1 �ɹ�
**************************************************************/
extern int BuildDNSDatabase();

/**************************************************************
* @brief	��database�зּ���������
* @param	domainName	     ����
* @param    ip			     ip��ַ������ֵ,�뱣֤����
                             ��16�ֽڵĿռ�(15���ַ�+\0)
* @return	0                û�ҵ�
* @return	1                �ҵ���
**************************************************************/
extern int FindInDNSDatabase(const char* domainName, char* ip);

/**************************************************************
* @brief	����λ��
* @param	domainName	     ����
* @param    cnt			     ��ͻ����
* @return	int              λ��
**************************************************************/
int Hash(const char* domainName, int cnt);

/**************************************************************
* @brief	��Ӽ�¼��DNSCache
* @param	domainName	     ����
* @param    ip			     ip
* @param    ttl              ttl
* @param    loc              ���λ��
* @return	void
**************************************************************/
extern void InsertHash(const char* domainName, const char* ip, int ttl, int loc);

/**************************************************************
* @brief	��DNSCache�в��Ҽ�¼
* @param	domainName	     ����
* @param    loc              λ��
                             (�ҵ�λ�û����λ��,ȡ����return)
* @return	0                δ�ҵ�
* @return   1                �ҵ���
**************************************************************/
extern int SearchHash(const char* domainName, char* ip, int* loc);

/**************************************************************
* @brief	����DNScache�����TTL��ɾ�����ڵ�DNS��¼
* @param	void
* @return   void
**************************************************************/
extern void UpdateCache();

/**************************************************************
* @brief	�������Ͷ�Ӧ��ip���뵽�ļ�
* @param	domainName	    ����
* @param    ip				ip
* @return   0				�ɹ�
**************************************************************/
extern int InsertIntoDNSTXT(const char* domainName, const char* ip);

/**************************************************************
* @brief	��cache�еĹ��ڼ�¼ɾ��
* @param	i				cache[i]
* @return   void
**************************************************************/
extern void DelFromDNSTXT(int i);

/**************************************************************
* @brief	�ܷ����ļ����ҵ������ļ�¼
* @param	name			����ѯ����
* @param	ip				ip
* @return	0                δ�ҵ�
* @return   1                �ҵ���
**************************************************************/
extern int FindIPByDNSinTXT(const char* name, char* ip);
