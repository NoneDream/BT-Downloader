#include "util.h"
#include "btdata.h"

//�����������.torrent�ļ���announce����
announce_url_t* parse_announce_url(char* announce)
{
  char* announce_ind;
  char port_str[6];  // �˿ں����Ϊ5λ����
  int port_len = 0; // �˿ں��е��ַ���
  int port;
  char* url;
  int url_len = 0;
  //annouuce����"/announce"��β
  announce_ind = strstr(announce,"/announce");//strstr(str1,str2) ���������ж��ַ���str2�Ƿ���str1���Ӵ�������ǣ���ú�������str2��str1���״γ��ֵĵ�ַ�����򣬷���NULL��
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
