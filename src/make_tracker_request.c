#include "util.h"
#include "btdata.h"

// 制作一个发送给Tracker的HTTP请求. 使用一些全局变量来填充请求中的参数,
// 如info_hash, peer_id以及有多少字节已上传和下载, 等等.
//
// 事件: 0 - started, 1 - stopped, 2 - completed
// 这些宏的定义见头文件btdata.h
// 这个函数返回HTTP请求消息, 消息的长度写入mlen
char* make_tracker_request(int event, int* mlen)
{
  // 分配一个很大的空间给MESG, 并填充它
  char *MESG;
  char* cur;
  int i;

  MESG = (char*)malloc(4096*sizeof(char));
  cur = MESG;
  strcpy(cur,"GET /announce?");
  cur += strlen("GET /announce?");
  //MESG:GET /announce?
  // 填入info_hash
  char hexdigs[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  strcpy(cur,"info_hash=");
  cur += strlen("info_hash=");
   //MESG:GET /announce?info_hash=
  for(i=0; i<5; i++)
  {
    int j;
    int part = reverse_byte_orderi(g_torrentdata->info_hash[i]);
    unsigned char* p = (unsigned char*)&part;
    for(j=0; j<4; j++)
    {
      if((p[j] > 0x2F && p[j] < 0x3A) || (p[j] > 0x40 && p[j] < 0x5B) ||
         (p[j] > 0x60 && p[j] < 0x7B))
      {
        *cur++ = p[j];//是数字或字母，则直接复制
      }
      else
      {
        *cur++ = '%';
        cur += sprintf(cur,"%02X",p[j]);//不是则要转义（全部符号都进行了转义）
      }
    }
  }
  //MESG:GET /announce?info_hash=<20字节SHA1值>
  // Peer id
  int num;
  strcpy(cur,"&peer_id=");
  cur += strlen("&peer_id=");
  //MESG:GET /announce?info_hash=<20字节SHA1值>&peer_id=
  for(i=0; i<20; i++)
  {
    *cur++ = '%';
    cur += sprintf(cur,"%02X",(unsigned char)g_my_id[i]);
  }
  //MESG:GET /announce?info_hash=<20字节SHA1值>&peer_id=<经过转义的20字节peer_id>
  // port
  strcpy(cur,"&port=");
  cur += strlen("&port=");
  cur += sprintf(cur,"%d",g_peerport);
  //MESG:GET /announce?info_hash=<20字节SHA1值>&peer_id=<经过转义的20字节peer_id>&port=<peerport>
  // uploaded
  strcpy(cur,"&uploaded=");
  cur += strlen("&uploaded=");
  cur += sprintf(cur,"%d",g_uploaded);

  // downloaded
  strcpy(cur,"&downloaded=");
  cur += strlen("&downloaded=");
  cur += sprintf(cur,"%d",g_downloaded);

  // left
  strcpy(cur,"&left=");
  cur += strlen("&left=");
  cur += sprintf(cur,"%d",g_left);

  switch(event) {
  case BT_STARTED:{
    strcpy(cur,"&event=");
    cur += strlen("&event=");
    strcpy(cur,"started");
    cur += strlen("started");
    break;
  }
  case BT_STOPPED:{
    strcpy(cur,"&event=");
    cur += strlen("&event=");
    strcpy(cur,"stopped");
    cur += strlen("stopped");
    break;
  }
  case BT_COMPLETED:{
    strcpy(cur,"&event=");
    cur += strlen("&event=");
    strcpy(cur,"completed");
    cur += strlen("completed");
    break;
  }
  default:break;// 除了上述情况以外, 不发送event参数
  }

  // ip
  strcpy(cur,"&ip=");
  cur += strlen("&ip=");
  strcpy(cur,g_my_ip);
  cur += strlen(g_my_ip);

  strcpy(cur," HTTP/1.0\r\n\r\n");
  cur += strlen(" HTTP/1.0\r\n\r\n");

  *cur = '\0';

  *mlen = cur - MESG;

  return MESG;
}
