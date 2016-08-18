#ifndef TRACKER_WIRE_H_INCLUDED
#define TRACKER_WIRE_H_INCLUDED

#include "bencode.h"

#define MAXLINE 4096

// Tracker HTTP响应的数据部分
typedef struct _tracker_response {
  int size;       // B编码字符串的字节数
  char* data;     // B编码的字符串
} tracker_response;

// 由tracker返回的响应中peers键的内容
typedef struct{
  char id[21]; // 20用于null终止符
  int port;
  char* ip; // Null终止
} peerdata;

// 包含在tracker响应中的数据
typedef struct{
  int interval;
  int numpeers;
  peerdata* peers;
}tracker_data;

//需要发送给tracker的数据
typedef struct{
  int info_hash[5];
  char peer_id[20];
  int port;
  int uploaded;
  int downloaded;
  int left;
  char ip[16]; // 自己的IP地址, 格式为XXX.XXX.XXX.XXX, 最后以'\0'结尾
} tracker_request;

///制作发送给tracker的请求

// 制作一个发送给Tracker的HTTP请求. 使用一些全局变量来填充请求中的参数,
// 如info_hash, peer_id以及有多少字节已上传和下载, 等等.
//
// 事件: 0 - started, 1 - stopped, 2 - completed
// 这些宏的定义见头文件btdata.h
// 这个函数返回HTTP请求消息, 消息的长度写入mlen
char* make_tracker_request(int event, int* mlen);

///处理tracker的响应

// 读取并处理来自Tracker的HTTP响应, 确认它格式正确, 然后从中提取数据.
// 一个Tracker的HTTP响应格式如下所示:
// HTTP/1.0 200 OK       (17个字节,包括最后的\r\n)
// Content-Length: X     (到第一个空格为16个字节) 注意: X是一个数字
// Content-Type: text/plain (26个字节)
// Pragma: no-cache (18个字节)
// \r\n  (空行, 表示数据的开始)
// data                  注意: 从这开始是数据, 但并没有一个data标签
tracker_response* preprocess_tracker_response(int sockfd);

// 解码B编码的数据, 将解码后的数据放入tracker_data结构
tracker_data* get_tracker_data(char* data, int len);

// 处理来自Tracker的字典模式的peer列表
void get_peers(tracker_data* td, be_node* peer_list);

// 给出一个peerdata的指针和一个peer的字典数据, 填充peerdata结构
void get_peer_data(peerdata* peer, be_node* ben_res);

#endif // TRACKER_WIRE_H_INCLUDED
