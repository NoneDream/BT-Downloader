#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/stat.h>
#include "peer_wire.h"
#include "global.h"
#include "util.h"

// ��ȷ�Ĺرտͻ���
void client_shutdown(int sig)
{
  // ����ȫ��ֹͣ������ֹͣ���ӵ�����peer, �Լ���������peer������. Set global stop variable so that we stop trying to connect to peers and
  // �����������peer���ӵ��׽��ֺ����ӵ�����peer���߳�.
  g_done = 0;
}

//���������������������ĸ�ʽ��������֮
void handle_stdin(int argc,char **argv,task_info_struct *task_info,host_info_struct *host_info){
    if(argc < 4){
        printf("Usage: SimpleTorrent <torrent file> <ip of this machine (XXX.XXX.XXX.XXX form)> <downloaded file location> [isseed]\n");
        printf("\t isseed is optional, 1 indicates this is a seed and won't contact other clients\n");
        printf("\t default:0 indicates a downloading client and will connect to other clients and receive connections\n");
        exit(1);
    }

    if( argc > 4 )task_info->isseed = !!atoi(argv[4]);

    if( task_info->iseed )printf("SimpleTorrent running as seed.\n");
    else printf("SimpleTorrent running as normal.\n");

    strncpy(task_info->downlocation,argv[3],strlen(argv[3]));
    //��֤����·���ַ�����β��'/'������֮���ʹ��
    if(task_info->downlocation[strlen(argv[3])] == '/'){
        task_info->downlocation[strlen(argv[3])+1] = '\0';
    }
    else{
        task_info->downlocation[strlen(argv[3])+1] = '/';
        task_info->downlocation[strlen(argv[3])+2] = '\0';
    }

    strncpy(host_info->my_ip,argv[2],strlen(argv[2]));
    host_info->my_ip[strlen(argv[2])+1] = '\0';

    task_info->torrentdata = parsetorrentfile(argv[1]);

    return;
}

//����������ڳ�ʼ��ȫ�ֱ����������ļ���飩
void init(task_info_struct *task_info,host_info_struct *host_info)
{
    int i;
    announce_url_t *tracker_url;
    struct hostent *host;
    //in_addr_t tracker_ip;

    //task_info->torrentdata = parsetorrentfile(argv[1]);

    //piece_num_to_save=(sem_t *)malloc(sizeof(sem_t));
    //packet_num_to_save=(sem_t *)malloc(sizeof(sem_t));
    //sem_init(piece_num_to_save,0,0);
    //sem_init(packet_num_to_save,0,0);

    //��ȡtraker��ip��ַ
    tracker_url=parse_announce_url(task_info->torrentdata->announce);
    host=gethostbyname(tracker_url->hostname);
    if(host==NULL){
        printf("Fail to get tracker's IP!\n");
        exit(-222);
    }
    memcpy(task_info->tracker_addr.sin_addr,host->h_addr_list[0],sizeof(in_addr_t));
    task_info->tracker_addr.sin_port = tracker_url->port;
    task_info->tracker_addr.sin_family = AF_INET;

    free(tracker_url->hostname);
    free(tracker_url);

    //SHA1(g_torrentdata->, g_torrentdata->piece_len, g_infohash);

    host_info->peerport = LISTENING_PORT;
    //g_done = 0;
    task_info->uploaded = 0;
    task_info->downloaded = 0;
    task_info->left = task_info->torrentdata->length;
    pthread_mutex_init(&task_info->peer_list_mutex, NULL);

    ///����peer_id
    int val[5];
    srand(time(NULL));
    for(i=0; i<5; i++){
        val[i] = rand();
    }
    memcpy(host_info->my_id,(char*)val,20);
    host_info->my_id[20]=0;

    ///��ʼ��piecefield
    task_info->piecefield = bitmap_init(task_info->torrentdata->num_pieces);

    file_check(task_info);

    return;
}

//�������������ip��ַת��Ϊ�ַ�����ʽ����ʽΪXXX.XXX.XXX.XXX, null��ֹ
//�ɹ�����1��ʧ�ܷ���0
int ip_to_string(in_addr_t ip,char *s)
{
    char *buf;
    struct in_addr *p=(struct in_addr *)&ip;

    buf=inet_ntoa (*p);
    if(buf==NULL)return 0;
    memcpy(s,buf,1+strlen(buf));
    return 1;
}

//������������������ӣ�peer��tracker��
int connect_to_host(char* ip, int port)
{
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
    perror("[Error][connect_to_host]Could not create socket");
    return(-1);
  }

  struct sockaddr_in addr;
  memset(&addr,0,sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(ip);
  addr.sin_port = htons(port);

  if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("[Error][connect_to_host]Connect failure");
    return(-1);
  }

  return sockfd;
}

//�������ͨ�����ص�socket����һ���˿�
int make_listen_port(int port)
{
  int sockfd;

  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(sockfd <0)
  {
    perror("Could not create socket");
    return 0;
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if(bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
  {
      perror("Could not bind socket");
      return 0;
  }

  if(listen(sockfd, 20) < 0)
  {
    perror("Error listening on socket");
    return 0;
  }

  return sockfd;
}

// recvline(int fd, char **line)
// ����: ���׽���fd����һ���ַ���
// ����: �׽���������fd, �����յ�������д��line
// ���: ��ȡ���ֽ���
int recvline(int fd, char **line)
{
  int retVal;
  int lineIndex = 0;
  int lineSize  = 128;

  *line = (char *)malloc(sizeof(char) * lineSize);

  if (*line == NULL)
  {
    perror("malloc");
    return -1;
  }

  while ((retVal = read(fd, *line + lineIndex, 1)) == 1)
  {
    if ('\n' == (*line)[lineIndex])
    {
      (*line)[lineIndex] = 0;
      break;
    }

    lineIndex += 1;

    /*
      �����õ��ַ�̫��, �����·����л���.
    */
    if (lineIndex > lineSize)
    {
      lineSize *= 2;
      char *newLine = realloc(*line, sizeof(char) * lineSize);

      if (newLine == NULL)
      {
        retVal    = -1; /* reallocʧ�� */
        break;
      }

      *line = newLine;
    }
  }

  if (retVal < 0)
  {
    free(*line);
    return -1;
  }
  #ifdef NDEBUG
  else
  {
    fprintf(stdout, "%03d> %s\n", fd, *line);
  }
  #endif

  return lineIndex;
}
/* End recvline */

// recvlinef(int fd, char *format, ...)
// ����: ���׽���fd�����ַ���.�������������ָ��һ����ʽ�ַ���, ��������洢��ָ���ı�����
// ����: �׽���������fd, ��ʽ�ַ���format, ָ�����ڴ洢������ݵı�����ָ��
// ���: ��ȡ���ֽ���
int recvlinef(int fd, char *format, ...)
{
  va_list argv;
  va_start(argv, format);

  int retVal = -1;
  char *line;
  int lineSize = recvline(fd, &line);

  if (lineSize > 0)
  {
    retVal = vsscanf(line, format, argv);
    free(line);
  }

  va_end(argv);

  return retVal;
}
/* End recvlinef */

//������������������Ĵ�С��
int reverse_byte_orderi(int i)
{
  unsigned char c1, c2, c3, c4;
  c1 = i & 0xFF;
  c2 = (i >> 8) & 0xFF;
  c3 = (i >> 16) & 0xFF;
  c4 = (i >> 24) & 0xFF;
  return ((int)c1 << 24) + ((int)c2 << 16) + ((int)c3 << 8) + c4;
}
