/*
* THIS FILE IS FOR TCP TEST
*/




/*
struct sockaddr_in {
short   sin_family;
u_short sin_port;
struct  in_addr sin_addr;
char    sin_zero[8];
};
*/




#include<vector>
using namespace std;
#include "sysInclude.h"
extern void tcp_DiscardPkt(char *pBuffer, int type);
extern void tcp_sendReport(int type);
extern void tcp_sendIpPkt(unsigned char *pData, UINT16 len, unsigned int  srcAddr, unsigned int dstAddr, UINT8 ttl);
extern int waitIpPacket(char *pBuffer, int timeout);
extern unsigned int getIpv4Address();
extern unsigned int getServerIpv4Address();
#define CLOSED 0
#define SYNSENT 1
#define ESTABLISHED 2
#define FINWAIT1 3
#define FINWAIT2 4
#define TIMEWAIT 5
int gSrcPort = 2007;
int gDstPort = 2006;
int gSeqNum = 0;
int gAckNum = 0;
int global_socket = 0;
struct TCB {
	unsigned int src_addr;
	unsigned int dst_addr;
	unsigned short src_port;
	unsigned short dst_port;
	unsigned int status;
	unsigned int seq;
	unsigned int ack;
	unsigned int socket;
};
struct tcp_header {
	unsigned short src_port;
	unsigned short dst_port;
	unsigned int seq;
	unsigned int ack;
	unsigned short length_and_type;
	unsigned short window_size;
	unsigned short checksum;
	unsigned short urg_ptr;
	void construct(TCB* tcb, int flag)
	{
		window_size = htons(1);
		if (flag == PACKET_TYPE_SYN)
		{
			seq = htonl(1);
			ack = htonl(0);
		}
		else
		{
			seq = htonl(tcb->seq);
			ack = htonl(tcb->ack);
		}
		checksum = htons(0);
		switch (flag) {
		case PACKET_TYPE_DATA:length_and_type = htons(0x5000); break;
		case PACKET_TYPE_SYN:length_and_type = htons(0x5002); break;
		case PACKET_TYPE_SYN_ACK:length_and_type = htons(0x5012); break;
		case PACKET_TYPE_ACK:length_and_type = htons(0x5010); break;
		case PACKET_TYPE_FIN:length_and_type = htons(0x5001); break;
		case PACKET_TYPE_FIN_ACK:length_and_type = htons(0x5011); break;
		}

	}
};


struct ip_fake_header
{
	unsigned int src_addr;
	unsigned int dst_addr;
	unsigned short protocol;
	unsigned short len;
};




vector<TCB*> TCB_table;




unsigned short checksum_compute(unsigned short* pBuff, int length, unsigned short* pFakeHeader) {
	unsigned short* tempBuff = (unsigned short*)pBuff;
	unsigned int sum = 0;
	for (int i = 0; i < length; i++)
	{
		sum += ntohs(tempBuff[i]);
		sum = (sum & 0xffff) + (sum >> 16);
	}
	unsigned short *tempFakeHeader = (unsigned short*)pFakeHeader;
	for (int i = 0; i < 6; i++)
	{
		sum += ntohs(tempFakeHeader[i]);
		sum = (sum & 0xffff) + (sum >> 16);
	}
	return (unsigned short)sum;
}




int stud_tcp_input(char *pBuff, unsigned short len, unsigned int srcAddr,
	unsigned int dstAddr) {
	//字节序转换
	tcp_header* p_header = (tcp_header*)pBuff;
	unsigned int src_addr = ntohl(dstAddr);
	unsigned int dst_addr = ntohl(srcAddr);
	unsigned short src_port = ntohs(p_header->dst_port);
	unsigned short dst_port = ntohs(p_header->src_port);
	unsigned int seq = ntohl(p_header->seq);
	unsigned int ack = ntohl(p_header->ack);
	unsigned int length_and_flags = ntohs(p_header->length_and_type);
	TCB* tcb = NULL;
	for (int i = 0; i < TCB_table.size(); i++)
	{
		if (TCB_table[i]->src_addr == src_addr && TCB_table[i]->dst_addr == dst_addr && TCB_table[i]->src_port == src_port && TCB_table[i]->dst_port == dst_port)
		{
			tcb = TCB_table[i];
		}
	}
	if (!tcb)
	{
		return -1;
	}

	ip_fake_header fkh;
	fkh.src_addr = srcAddr;
	fkh.dst_addr = dstAddr;
	fkh.protocol = htons(6);
	fkh.len = htons(len+20);
	if (checksum_compute((char*)pBuff, len, (char*)&fkh) != 0)
	{
		return -1;
	}




	//检查序列号
	if (tcb->ack != seq)
	{
		tcp_DiscardPkt(pBuff, STUD_TCP_TEST_SEQNO_ERROR);
		return -1;
	}




	//有限状态机对报文的处理
	bool if_ACK = length_and_flags & 0x10;
	bool if_SYN = length_and_flags & 0x2;
	bool if_FIN = length_and_flags & 0x1;
	if (tcb->status == SYNSENT && if_SYN && if_ACK)
	{
		tcb->seq++;
		tcb->ack++;
		stud_tcp_output(NULL, 0, PACKET_TYPE_ACK, tcb->src_port, tcb->dst_port, tcb->src_addr, tcb->dst_addr);
		tcb->status = ESTABLISHED;
		return 0;
	}
	if (tcb->status == ESTABLISHED && if_ACK)
	{
		tcb->seq = ack;
		tcb->ack = seq + 1;
		return 0;
	}
	if (tcb->status == FINWAIT1 && if_ACK)
	{
		tcb->status = FINWAIT2;
		return 0;
	}
	if (tcb->status == FINWAIT2 && if_ACK && if_FIN)
	{
		tcb->seq++;
		tcb->ack++;
		stud_tcp_output(NULL, 0, PACKET_TYPE_ACK, tcb->src_port, tcb->dst_port, tcb->src_addr, tcb->dst_addr);
		tcb->status = TIMEWAIT;
		return 0;
	}
}




void stud_tcp_output(char *pData, unsigned short len, unsigned char flag, unsigned short srcPort, unsigned short dstPort, unsigned int srcAddr, unsigned int dstAddr)
{
	TCB* tcb = NULL;
	bool find_tcb = 0;
	if (TCB_table.size() == 0) {
		tcb = new TCB;
		TCB_table.push_back(tcb);
	}
	else {
		for (int i = 0; i < TCB_table.size(); i++) {
			if (TCB_table[i]->dst_addr == dstAddr && TCB_table[i]->src_addr == srcAddr
				&& TCB_table[i]->src_port == srcPort && TCB_table[i]->dst_port == dstPort) {
				find_tcb = 1;
				tcb = TCB_table[i];
			}
		}
	}
	if (tcb == NULL) return;


	ip_fake_header* fake_header = new(ip_fake_header);
	fake_header->src_addr = htonl(srcAddr);
	fake_header->dst_addr = htonl(dstAddr);
	fake_header->protocol = htons(6);
	fake_header->len = htons(20 + len);


	tcp_header* header = new(tcp_header);
	header->src_port = htons(srcPort);
	header->dst_port = htons(dstPort);
	header->construct(tcb, flag);
	if (flag == PACKET_TYPE_DATA) {
		memcpy((char *)header + 20, pData, len);
	}
	header->checksum = htons(~checksum_compute((unsigned short *)header, 10 + len / 2, (unsigned short *)fake_header));

	tcp_sendIpPkt((unsigned char*)header, len + 20, srcAddr, dstAddr, 255);

	if (flag == PACKET_TYPE_SYN)
	{
		tcb->src_addr = srcAddr;
		tcb->dst_addr = dstAddr;
		tcb->src_port = srcPort;
		tcb->dst_port = dstPort;
		tcb->seq = 1;
		tcb->ack = 1;
		tcb->status = SYNSENT;
	}
	if (flag == PACKET_TYPE_FIN_ACK && tcb->status == ESTABLISHED)
		tcb->status = FINWAIT1;
}


//(4)的内容
int stud_tcp_socket(int domain, int type, int protocol)
{
	TCB* tcb = new TCB;
	global_socket++;
	tcb->socket = global_socket;
	TCB_table.push_back(tcb);
	return global_socket;
}




int stud_tcp_connect(int sockfd, struct sockaddr_in *addr, int addrlen)
{
	TCB * tcb = NULL;
	for (int i = 0; i < TCB_table.size(); i++)
	{
		if (TCB_table[i]->socket == sockfd)
			tcb = TCB_table[i];
	}
	tcb->src_addr = getIpv4Address();
	tcb->dst_addr = getServerIpv4Address();
	tcb->src_port = gSrcPort;
	tcb->dst_port = ntohs(addr->sin_port);
	stud_tcp_output(NULL, 0, PACKET_TYPE_SYN, tcb->src_port, tcb->dst_port, tcb->src_addr, tcb->dst_addr);
	char * pBuffer = new char[250];
	int len = waitIpPacket(pBuffer, 255);
	if (len == -1)
		return -1;
	stud_tcp_input(pBuffer, len, htonl(tcb->dst_addr), htonl(tcb->src_addr));

	return 0;
}




int stud_tcp_send(int sockfd, const unsigned char *pData, unsigned short datalen, int flags)
{
	TCB * tcb = NULL;
	for (int i = 0; i < TCB_table.size(); i++)
	{
		if (TCB_table[i]->socket == sockfd)
			tcb = TCB_table[i];
	}
	if (tcb->status != ESTABLISHED)
	{
		return -1;
	}
	stud_tcp_output((char*)pData, datalen, PACKET_TYPE_DATA, tcb->src_port, tcb->dst_port, tcb->src_addr, tcb->dst_addr);
	char * pBuffer = new char[250];
	int len = waitIpPacket(pBuffer, 250);
	if (len == -1)
		return -1;
	else
		stud_tcp_input(pBuffer, len, htonl(tcb->dst_addr), htonl(tcb->src_addr));
	return 0;
}




int stud_tcp_recv(int sockfd, unsigned char *pData, unsigned short datalen, int flags)
{
	TCB * tcb = NULL;
	for (int i = 0; i < TCB_table.size(); i++)
	{
		if (TCB_table[i]->socket == sockfd)
			tcb = TCB_table[i];
	}
	if (tcb->status != ESTABLISHED)
		return -1;
	char* pBuffer = new char[250];
	int len = waitIpPacket(pBuffer, 250);
	if (len == -1)
		return -1;
	else {
		tcp_header *header = new tcp_header;
		memcpy((char*)header, pBuffer, 20);
		memcpy((char*)pData, (unsigned char *)pBuffer + 20, len - 20);
		tcb->seq = ntohl(header->ack);
		tcb->ack = ntohl(header->seq) + len - 20;
		stud_tcp_output(NULL, 0, PACKET_TYPE_ACK, tcb->src_port, tcb->dst_port, tcb->src_addr, tcb->dst_addr);
	}
	return 0;
}




int stud_tcp_close(int sockfd)
{
	TCB *tcb = NULL;
	int index = 0;
	for (int i = 0; i < TCB_table.size(); i++) {
		if (TCB_table[i]->socket == sockfd)
		{
			tcb = TCB_table[i];
			index = i;
		}
	}
	if (tcb->status == SYNSENT)
	{
		TCB_table.erase(TCB_table.begin() + index);
		return -1;
	}


	stud_tcp_output(NULL, 0, PACKET_TYPE_FIN_ACK, tcb->src_port, tcb->dst_port, tcb->src_addr, tcb->dst_addr);
	tcb->seq++;
	tcb->ack++;


	char* pBuffer = new char[250];
	int len = waitIpPacket(pBuffer, 250);
	if (len == -1) return -1;
	stud_tcp_input(pBuffer, len, htonl(tcb->dst_addr), htonl(tcb->src_addr));


	len = waitIpPacket(pBuffer, 255);
	if (len == -1) return -1;
	stud_tcp_output(NULL, 0, PACKET_TYPE_ACK, tcb->src_port, tcb->dst_port, tcb->src_addr, tcb->dst_addr);

	return 0;
}