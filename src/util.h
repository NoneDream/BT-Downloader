
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

// 正确的关闭客户端
void client_shutdown(int sig);

//这个函数负责检查输入变量的格式，并解析之
void handle_stdin(int argc,char **argv);

//这个函数用于初始化全局变量（包括文件检查）
void init();

//这个函数用来把ip地址转换为字符串形式：格式为XXX.XXX.XXX.XXX, null终止
//成功返回1，失败返回0
int ip_to_string(in_addr_t ip,char *s);

//这个函数用来建立链接（peer或tracker）
int connect_to_host(char* ip, int port);

//这个函数通过返回的socket监听一个端口
int make_listen_port(int port);

// recvline(int fd, char **line)
// 描述: 从套接字fd接收一行字符串
// 输入: 套接字描述符fd, 将接收的行数据写入line
// 输出: 读取的字节数
int recvline(int fd, char **line);

// recvlinef(int fd, char *format, ...)
// 描述: 从套接字fd接收字符串.这个函数允许你指定一个格式字符串, 并将结果存储在指定的变量中
// 输入: 套接字描述符fd, 格式字符串format, 指向用于存储结果数据的变量的指针
// 输出: 读取的字节数
int recvlinef(int fd, char *format, ...);

//这个函数交换整形数的大小端
int reverse_byte_orderi(int i);

#endif
