
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

#include "btdata.h"
#include "bencode.h"

#ifndef UTIL_H
#define UTIL_H

// ��ȷ�Ĺرտͻ���
void client_shutdown(int sig);

//���������������������ĸ�ʽ��������֮
void handle_stdin(int argc,char **argv);

//����������ڳ�ʼ��ȫ�ֱ����������ļ���飩
void init();

//�������������ip��ַת��Ϊ�ַ�����ʽ����ʽΪXXX.XXX.XXX.XXX, null��ֹ
//�ɹ�����1��ʧ�ܷ���0
int ip_to_string(in_addr_t ip,char *s);

//������������������ӣ�peer��tracker��
int connect_to_host(char* ip, int port);

//�������ͨ�����ص�socket����һ���˿�
int make_listen_port(int port);

// recvline(int fd, char **line)
// ����: ���׽���fd����һ���ַ���
// ����: �׽���������fd, �����յ�������д��line
// ���: ��ȡ���ֽ���
int recvline(int fd, char **line);

// recvlinef(int fd, char *format, ...)
// ����: ���׽���fd�����ַ���.�������������ָ��һ����ʽ�ַ���, ��������洢��ָ���ı�����
// ����: �׽���������fd, ��ʽ�ַ���format, ָ�����ڴ洢������ݵı�����ָ��
// ���: ��ȡ���ֽ���
int recvlinef(int fd, char *format, ...);

//������������������Ĵ�С��
int reverse_byte_orderi(int i);

#endif
