#ifndef __FILES__
#define __FILES__

#include <stdio.h>
#include <pthread.h>

//多文件模式下，一个文件的信息
typedef struct _file_info{
  long long length;       // 文件长度
  char* path;     // 文件路径
  int new_flag; //文件是否是新建的（新建的则不需要检查）
  int piece_start;//文件起始位置所在的分片序号
  int piece_end;//文件结束位置所在的分片序号
  int piece_offset;//文件相对整的分片的偏移（以头部为准9+）
  FILE *f;//文件指针
  pthread_mutex_t file_mutex;//文件操作的互斥锁
}file_info;

//这个函数检查所有文件是否存在，若不存在则创建
int file_check();

//这个函数返回文件大小
int file_size(char* filename);

// 计算一个打开文件的字节数
int file_len(FILE* fp);

//这个函数用来获取一个分片的数据，输入指针为分片起始位置的文件的指针
int get_piece(char *data,file_info **pp,int piece_num,int len);///若输入len为0，则函数自动判断长度

//这个函数用来保存一个分片的数据，输入指针为分片起始位置的文件的指针
int save_piece(char *data,file_info **pp,int piece_num,int len);///若输入len为0，则函数自动判断长度

//这个线程负责将下载完成的分片保存至磁盘文件
void *save_to_disk(void *arg);

#endif
