#include "util.h"
#include "btdata.h"

//这个函数解析.torrent文件中announce部分
announce_url_t* parse_announce_url(char* announce)
{
  char* announce_ind;
  char port_str[6];  // 端口号最大为5位数字
  int port_len = 0; // 端口号中的字符数
  int port;
  char* url;
  int url_len = 0;
  //annouuce键以"/announce"结尾
  announce_ind = strstr(announce,"/announce");//strstr(str1,str2) 函数用于判断字符串str2是否是str1的子串。如果是，则该函数返回str2在str1中首次出现的地址；否则，返回NULL。
  announce_ind--;
  while(*announce_ind != ':')
  {
    port_len++;
    announce_ind--;
  }
  strncpy(port_str,announce_ind+1,port_len);
  port_str[port_len+1] = '\0';
  port = atoi(port_str);

  char* p;
  url_len=announce_ind-announce;
  /*for(p=announce; p<announce_ind; p++)
  {
    url_len++;
  }*/

  announce_url_t* ret;
  ret = (announce_url_t *)malloc(sizeof(announce_url_t));
  if(ret == NULL)
  {
    perror("Could not allocate announce_url_t");
    exit(-73);
  }

  p = announce;
  printf("ANNOUNCE: %s\n",announce);
  if(strstr(announce,"http://") > 0)
  {
    url_len -= 7;
    p += 7;
  }

  url = (char*)malloc((url_len+1)*sizeof(char)); // +1 for \0
  strncpy(url,p,url_len);
  url[url_len+1] = '\0';

  ret->hostname = url;
  ret->port = port;

  return ret;
}
