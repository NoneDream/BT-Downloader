#include <malloc.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include "tracker_wire.h"
#include "global.h"

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
  //char hexdigs[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  strcpy(cur,"info_hash=");
  cur += strlen("info_hash=");
   //MESG:GET /announce?info_hash=

    int j;
    //int part = reverse_byte_orderi(g_torrentdata->info_hash[i]);
    unsigned char* p = g_torrentdata->info_hash;
    for(j=0; j<20; j++)
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
  //MESG:GET /announce?info_hash=<20字节SHA1值>
  // Peer id
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

tracker_response* preprocess_tracker_response(int sockfd)
{
   char rcvline[MAXLINE];
   char tmp[MAXLINE];
   char* data;
   int len;
   //int offset = 0;
   int datasize = -1;
   printf("Reading tracker response...\n");
   // HTTP LINE
   len = recv(sockfd,rcvline,17,MSG_WAITALL);
   if(len < 0)
   {
     perror("Error, cannot read socket from tracker");
     exit(-6);
   }
   strncpy(tmp,rcvline,17);

   if(strncmp(tmp,"HTTP/1.1 200 OK\r\n",strlen("HTTP/1.1 200 OK\r\n")))
   {
     perror("Error, didn't match HTTP line");
     exit(-6);
   }
   memset(rcvline,0xFF,MAXLINE);
   memset(tmp,0x0,MAXLINE);
   /*
   //Content-Type
   len = recv(sockfd,rcvline,26,0);
   if(len <= 0)
   {
     perror("Error, cannot read socket from tracker");
     exit(-6);
   }
   strncpy(tmp,rcvline,26);
   if(strncmp(tmp,"Content-Type: ",strlen("Content-Type: ")))
   {
     perror("Error, didn't match Content-Type line");
     exit(-6);
   }
   memset(rcvline,0xFF,MAXLINE);
   memset(tmp,0x0,MAXLINE);
   */
   //Contenct-type
   len = recv(sockfd,rcvline,26,MSG_WAITALL);
   if(len <= 0)
   {
     perror("Error, cannot read socket from tracker");
     exit(-6);
   }
   // Content-Length
   len = recv(sockfd,rcvline,16,MSG_WAITALL);
   if(len <= 0)
   {
     perror("Error, cannot read socket from tracker");
     exit(-6);
   }
   strncpy(tmp,rcvline,16);
   if(strncmp(tmp,"Content-Length: ",strlen("Content-Length: ")))
   {
     perror("Error, didn't match Content-Length line");
     exit(-6);
   }
   memset(rcvline,0xFF,MAXLINE);
   memset(tmp,0x0,MAXLINE);
   // 读取Content-Length的数据部分
   char c[2];
   char num[MAXLINE];
   int count = 0;
   c[0] = 0; c[1] = 0;
   while(c[0] != '\r' && c[1] != '\n')
   {
      len = recv(sockfd,rcvline,1,MSG_WAITALL);
      if(len <= 0)
      {
        perror("Error, cannot read socket from tracker");
        exit(-6);
      }
      num[count] = rcvline[0];
      c[0] = c[1];
      c[1] = num[count];
      count++;
   }
   datasize = atoi(num);
   //printf("NUMBER RECEIVED: %d\n",datasize);
   memset(rcvline,0xFF,MAXLINE);
   memset(num,0x0,MAXLINE);
   // 读取Content-type和Pragma行
   /*
   len = recv(sockfd,rcvline,18,0);
   if(len <= 0)
   {
     perror("Error, cannot read socket from tracker");
     exit(-6);
   }
   */
   /// 去除响应中额外的\r\n空行
   len = recv(sockfd,rcvline,2,MSG_WAITALL);
   if(len <= 0)
   {
     perror("Error, cannot read socket from tracker");
     exit(-6);
   }

   /// 分配空间并读取数据, 为结尾的\0预留空间
   int i;
   data = (char*)malloc((datasize+1)*sizeof(char));
   printf("Size:%d\n",datasize);
   for(i=0; i<datasize; i++)
   {
      len = recv(sockfd,data+i,1,MSG_WAITALL);
      if(len < 0)
      {
        perror("Error, cannot read socket from tracker");
        exit(-6);
      }
   }
   data[datasize] = '\0';

   for(i=0; i<datasize; i++)
     printf("%c",data[i]);
   printf("\n");

   /// 分配, 填充并返回tracker_response结构.
   tracker_response* ret;
   ret = (tracker_response*)malloc(sizeof(tracker_response));
   if(ret == NULL)
   {
     printf("Error allocating tracker_response ptr\n");
     return 0;
   }
   ret->size = datasize;
   ret->data = data;

   return ret;
}

tracker_data* get_tracker_data(char* data, int len)
{
  tracker_data* ret;
  be_node* ben_res;
  ben_res = be_decoden(data,len);
  if(ben_res->type != BE_DICT)
  {
    perror("Data not of type dict");
    exit(-12);
  }

  ret = (tracker_data*)malloc(sizeof(tracker_data));
  if(ret == NULL)
  {
    perror("Could not allcoate tracker_data");
    exit(-12);
  }

  // 遍历键并测试它们
  int i;
  for (i=0; ben_res->val.d[i].val != NULL; i++)
  {
    //printf("%s\n",ben_res->val.d[i].key);
    // 检查是否有失败键
    if(!strncmp(ben_res->val.d[i].key,"failure reason",strlen("failure reason")))
    {
      printf("Error: %s",ben_res->val.d[i].val->val.s);
      exit(-12);
    }
    // interval键
    if(!strncmp(ben_res->val.d[i].key,"interval",strlen("interval")))
    {
      ret->interval = (int)ben_res->val.d[i].val->val.i;
    }
    // peers键
    if(!strncmp(ben_res->val.d[i].key,"peers",strlen("peers")))
    {
      be_node* peer_list = ben_res->val.d[i].val;
      get_peers(ret,peer_list);
    }
  }

  be_free(ben_res);

  return ret;
}

void get_peers(tracker_data* td, be_node* peer_list)
{
  int i;
  int numpeers = 0;
  unsigned char *p;

  if(peer_list->type==BE_STR)
  {
	numpeers=peer_list->len/6;
	p=(unsigned char *)peer_list->val.s;

  	td->numpeers = numpeers;
  	td->peers = (peerdata*)malloc(numpeers*sizeof(peerdata));
  	if(td->peers == NULL)
  	{
  	  perror("Couldn't allocate peers");
  	  exit(-12);
  	}

	for(i=0;i<numpeers;i++)
	{
		td->peers[i].ip=malloc(17);
		sprintf(td->peers[i].ip,"%u.%u.%u.%u\n",(int)p[0],p[1],p[2],p[3]);
		td->peers[i].port= ntohs(*(uint16_t *)(p+4));/// ((unsigned int)p[4])+(((unsigned int)p[5])<<8);
		p+=6;
	}
  }

  return;
}

void get_peer_data(peerdata* peer, be_node* ben_res)
{
  int i;

  if(ben_res->type != BE_DICT)
  {
    perror("Don't have a dict for this peer");
    exit(-12);
  }

  // 遍历键并填充peerdata结构
  for (i=0; ben_res->val.d[i].val != NULL; i++)
  {
    //printf("%s\n",ben_res->val.d[i].key);

    // peer id键
    if(!strncmp(ben_res->val.d[i].key,"peer id",strlen("peer id")))
    {
      //printf("Peer id: %s\n", ben_res->val.d[i].val->val.s);
      memcpy(peer->id,ben_res->val.d[i].val->val.s,20);
      peer->id[20] = '\0';
      /*
      int idl;
      printf("Peer id: ");
      for(idl=0; idl<len; idl++)
        printf("%02X ",(unsigned char)peer->id[idl]);
      printf("\n");
      */
    }
    // ip键
    if(!strncmp(ben_res->val.d[i].key,"ip",strlen("ip")))
    {
      int len;
      //printf("Peer ip: %s\n",ben_res->val.d[i].val->val.s);
      len = strlen(ben_res->val.d[i].val->val.s);
      peer->ip = (char*)malloc((len+1)*sizeof(char));
      strcpy(peer->ip,ben_res->val.d[i].val->val.s);
    }
    // port键
    if(!strncmp(ben_res->val.d[i].key,"port",strlen("port")))
    {
      //printf("Peer port: %d\n",ben_res->val.d[i].val->val.i);
      peer->port = ben_res->val.d[i].val->val.i;
    }
  }
}
