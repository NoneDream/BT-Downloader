#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <openssl/sha.h>
#include "util.h"
#include "bencode.h"
#include "files.h"
#include "parsetorrentfile.h"

#define DEFINE
#include "global.h"

//#define MAXLINE 4096
// pthread数据
host_info_struct host_info;
task_info_struct task_info;

int main(int argc, char **argv)
{
// 注意: 你可以根据自己的需要修改这个程序的使用方法

  handle_stdin(argc,argv);//检查输入参数，并解析
  init();//初始化全局变量，检查分片完成情况

  announce_url_t* announce_info;
  announce_info = parse_announce_url(g_torrentdata->announce);//解析torrent文件announce部分
  // 提取tracker url中的IP地址
  printf("HOSTNAME: %s\n",announce_info->hostname);
  struct hostent *record;
  record = gethostbyname(announce_info->hostname);
  if (record==NULL)
  {
    printf("gethostbyname(%s) failed", announce_info->hostname);
    exit(1);
  }
  struct in_addr* address;
  address =(struct in_addr * )record->h_addr_list[0];
  printf("Tracker IP Address: %s\n", inet_ntoa(* address));
  strcpy(g_tracker_ip,inet_ntoa(*address));
  g_tracker_port = announce_info->port;

  free(announce_info);
  announce_info = NULL;

  // 初始化
  // 设置信号句柄
  signal(SIGINT,client_shutdown);

  // 设置监听peer的线程

  /// 定期联系Tracker服务器
  int mlen;
  int sockfd;
  int i;
  tracker_response *tr;
  tracker_data *response;
  char *MESG;
  MESG = make_tracker_request(BT_STARTED,&mlen);
  while(!g_done){

    ///打印MESG

    printf("MESG: ");
    for(i=0; i<mlen; i++)
        printf("%c",MESG[i]);
    printf("\n");

    ///创建套接字并发送报文给Tracker

    printf("Creating socket to tracker...\n");
    sockfd = connect_to_host(g_tracker_ip, g_tracker_port);
    printf("Sending request to tracker...\n");
    send(sockfd, MESG, mlen, 0);

    /// 读取并预处理来自Tracker的响应

    tr = preprocess_tracker_response(sockfd);

    /// 关闭套接字, 以便再次使用

    shutdown(sockfd,SHUT_RDWR);
    close(sockfd);
    sockfd = 0;

    ///解码来自tracker的响应

    response = get_tracker_data(tr->data,tr->size);
    printf("Num Peers: %d\n",response->numpeers);
    for(i=0; i<response->numpeers; i++)
    {
      printf("Peer id: ");
      int idl;
      for(idl=0; idl<20; idl++)printf("%02X ",(unsigned char)response->peers[i].id[idl]);
      printf("\n");
      printf("Peer ip: %s\n",response->peers[i].ip);
      printf("Peer port: %d\n",response->peers[i].port);
    }

    ///添加新的peer

    connect_to_peers(response);

    ///必须等待td->interval秒, 然后再发出下一个GET请求

    sleep(response->interval);

    ///释放动态分配的内存

    free(tr->data);
    free(tr);
    free(response->peers);
    free(response);

    ///重新创建MESG

    free(MESG);
    MESG = make_tracker_request(-1,&mlen);// -1 指定不发送event参数
  }

  // 睡眠以等待其他线程关闭它们的套接字, 只有在用户按下ctrl-c时才会到达这里
  sleep(2);

  exit(0);
}
