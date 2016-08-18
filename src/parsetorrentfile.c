#include <openssl/sha.h>
#include <string.h>
#include "parsetorrentfile.h"
#include "files.h"
#include "bencode.h"
#include "util.h"
#include "global.h"

//这个函数解析.torrent文件中announce部分
announce_url_t* parse_announce_url(char* announce)
{
  char* announce_ind;
  char port_str[6];  // 端口号最大为5位数字
  int port_len = 0; // 端口号中的字符数
  int port;
  char* url;
  int url_len = 0;
  char* p;

  p = announce;
  printf("ANNOUNCE: %s\n",announce);

  //annouuce键以"/announce"结尾
  announce_ind = strstr(announce,"/announce");//strstr(str1,str2) 函数用于判断字符串str2是否是str1的子串。如果是，则该函数返回str2在str1中首次出现的地址；否则，返回NULL。
  if(announce_ind==NULL){
    printf("[Error][parse_announce_url]Invalid 'announce':have no '/announce' string!\n");
    exit(23);
  }
  announce_ind--;
  if(*announce_ind<'0'||*announce_ind>'9'){
    printf("[Error][parse_announce_url]Invalid 'announce':don't have port num!\n");
    exit(24);
  }
  while(*announce_ind != ':')
  {
    port_len++;
    announce_ind--;
  }
  if(port_len>5){
    printf("[Error][parse_announce_url]Invalid 'announce':port num is too long(%d)!\n",port_len);
    exit(24);
  }
  strncpy(port_str,announce_ind+1,port_len);
  port_str[port_len] = '\0';
  port = atoi(port_str);

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

// 注意: 这个函数可以处理多文件模式torrent
torrentmetadata_t* parsetorrentfile(char* filename)
{
  int i;
  be_node* ben_res;
  FILE* f;
  int flen;
  char* data;
  torrentmetadata_t* ret;

  // 打开文件, 获取数据并解码字符串
  f = fopen(filename,"r");
  if(f==NULL){
    printf("[Error][parsetorrentfile]Fail to open torrent file!\n");
    exit(2);
  }
  flen = file_len(f);
  data = (char*)malloc(flen*sizeof(char));
  fread((void*)data,sizeof(char),flen,f);
  fclose(f);
  ben_res = be_decoden(data,flen);

  // 遍历节点, 检查文件结构并填充相应的字段.
  if(ben_res->type != BE_DICT)
  {
    perror("Torrent file isn't a dictionary");
    exit(-13);
  }

  ret = (torrentmetadata_t *)malloc(sizeof(torrentmetadata_t));
  memset(ret,0,sizeof(torrentmetadata_t));
  if(ret == NULL)
  {
    perror("Could not allocate torrent meta data");
    exit(-13);
  }

  // 计算这个torrent的info_hash值
  // 注意: SHA1函数返回的哈希值存储在一个整数数组中, 对于小端字节序主机来说,
  // 在与tracker或其他peer返回的哈希值进行比较时, 需要对本地存储的哈希值
  // 进行字节序转换. 当你生成发送给tracker的请求时, 也需要对字节序进行转换.
  char* info_loc, *info_end;
  info_loc = strstr(data,"infod");  // 查找info键, 它的值是一个字典
  info_loc += 4; // 将指针指向值开始的地方（指向'd'）
  info_end = data+flen-1;
  // 去掉结尾的e
  if(*info_end == 'e')
  {
    --info_end;
  }

  int len = 0;
  len=info_end-info_loc;
  //for(p=info_loc; p<=info_end; p++) len++;

  // 计算上面字符串的SHA1哈希值以获取info_hash
  /*SHA1Context sha;
  SHA1Reset(&sha);
  SHA1Input(&sha,(const unsigned char*)info_loc,len);
  if(!SHA1Result(&sha))
  {
    printf("Make info_hash failed!\n");
  }
  memcpy(ret->info_hash,sha.Message_Digest,20);*/
  SHA1((const unsigned char *)info_loc, len,(unsigned char *)ret->info_hash);
  printf("SHA1: ");
  for(i=0; i<20; i++)
  {
    printf("%02X",ret->info_hash[i]);
  }
  printf("\n");

  // 检查键并提取对应的信息
  int filled=0;
  ret->type=TYPE_UNKNOWN;
  for(i=0; ben_res->val.d[i].val != NULL; i++)
  {
    int j;
    if(!strncmp(ben_res->val.d[i].key,"announce",1+strlen("announce")))
    {
	  ret->announce = (char *)malloc(strlen(ben_res->val.d[i].val->val.s)*sizeof(char));
	  strcpy(ret->announce,ben_res->val.d[i].val->val.s);
      filled++;
    }
    // info是一个字典, 它还有一些其他我们关心的键
    else if(!strncmp(ben_res->val.d[i].key,"info",strlen("info")))
    {
      be_dict* idict;
      if(ben_res->val.d[i].val->type != BE_DICT)
      {
        perror("Expected 'info' to be a dict, but got something else");
        exit(-3);
      }
      idict = ben_res->val.d[i].val->val.d;
      // 检查这个字典的键
      for(j=0; idict[j].key != NULL; j++)
      {
        if(!strncmp(idict[j].key,"length",strlen("length")))
        {
          if(ret->type==TYPE_UNKNOWN)ret->type=TYPE_ONE_FILE;
          else{
            printf("[Error][parsetorrentfile]Confusing:have both key 'length' and key 'files'.\n");
            exit(-22);
          }
          ret->length = idict[j].val->val.i;
          filled++;
        }
        else if(!strncmp(idict[j].key,"files",strlen("files")))
        {
            ret->length=0;
          if(ret->type==TYPE_UNKNOWN)ret->type=TYPE_FILES;
          else{
            printf("[Error][parsetorrentfile]Confusing:have both key 'length' and key 'files'.\n");
            exit(-22);
          }
          ////////////////////////////////////////////////////////////////////////////////////-------------------------待测试：多文件的解析
          ret->files=malloc(sizeof(file_info));
          ret->files->path=NULL;
          ret->files->length=0;
          be_node **file_list=idict[j].val->val.l;
          be_dict *file_dict;
          int x,y,k,len_path,len_add;
          for(x=0;file_list[x]!=NULL;++x){
            file_dict=file_list[x]->val.d;
            for(y=0;file_dict[y].key!=NULL;++y){
              if(!strncmp(file_dict[y].key,"length",strlen("length"))){
                ret->files=realloc(ret->files,(x+2)*sizeof(file_info));
                ret->files[x].length=file_dict[y].val->val.i;
                ret->length+=ret->files[x].length;
              }
              else if(!strncmp(file_dict[y].key,"path",strlen("path"))){
                ret->files=realloc(ret->files,(x+2)*sizeof(file_info));
                ret->files[x].path=malloc(1+strlen(g_downlocation));
                strcpy(ret->files[x].path,g_downlocation);
                len_path=strlen(g_downlocation);
                //ret->files[x].path[len_path-1]='\0';

                for(k=0;file_dict[y].val->val.l[k]!=NULL;++k){
                  len_add=1+be_str_len(file_dict[y].val->val.l[k]);
                  ret->files[x].path=realloc(ret->files[x].path,(len_path+len_add)*sizeof(char));
                  strcpy(ret->files[x].path+len_path,file_dict[y].val->val.l[k]->val.s);
                  /*if(-1==sprintf(ret->files[x].path+len_path,"%s",file_dict[y].val->val.l[k]->val.s)){
                    printf("Fail to write to path!\n");
                  }*/
                  len_path+=len_add;
                  ret->files[x].path[len_path-1]='/';
                }
                ret->files[x].path[len_path-1]='\0';
              }
            }
            ret->files[x].f=NULL;
          }
          ret->files[x].length=0;
          ret->files[x].path=NULL;
          ret->files[x].f=NULL;
          ////////////////////////////////////////////////////////////////////////////////////
          filled+=2;
        }
        else if(!strncmp(idict[j].key,"name",strlen("name")))
        {
          if(ret->type==TYPE_FILES)continue;
          ret->name = (char*)malloc((1+strlen(idict[j].val->val.s))*sizeof(char));
          strcpy(ret->name,idict[j].val->val.s);
          filled++;
        }
        else if(!strncmp(idict[j].key,"piece length",strlen("piece length")))
        {
          ret->piece_len = idict[j].val->val.i;
          filled++;
        }
        else if(!strncmp(idict[j].key,"pieces",strlen("pieces")))
        {
          int num_pieces = ret->length/ret->piece_len;
          if(ret->length % ret->piece_len != 0)
          num_pieces++;
          ret->pieces = (char *)malloc(num_pieces*20*sizeof(char));
          memcpy(ret->pieces,idict[j].val->val.s,num_pieces*20);
          ret->num_pieces = num_pieces;
          filled++;
        }

      } // for循环结束
    } // info键处理结束
  }

  ///对于单文件种子，也填充files键
  if(ret->type==TYPE_ONE_FILE){
    ret->files=malloc(2*sizeof(file_info));
    ret->files[0].path=malloc(strlen(ret->name)+strlen(g_downlocation)+1);
    strcpy(ret->files[0].path,g_downlocation);
    strcpy(ret->files[0].path+strlen(g_downlocation),ret->name);
    ret->files[0].length=ret->length;
    ret->files[1].path=NULL;
  }

  // 确认已填充了必要的字段

  be_free(ben_res);

  if(filled < 5)
  {
    printf("Did not fill necessary field\n");
    return NULL;
  }
  else
    return ret;
}
