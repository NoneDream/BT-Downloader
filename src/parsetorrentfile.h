#ifndef PARSETORRENTFILE_H_INCLUDED
#define PARSETORRENTFILE_H_INCLUDED

#include "files.h"

// 包含在announce url中的数据(例如, 主机名和端口号)
typedef struct _announce_url_t{
  char *hostname;
  int port;
} announce_url_t;

// 元信息文件中包含的数据
// 元信息文件中包含的数据
#define TYPE_UNKNOWN 0
#define TYPE_ONE_FILE 1
#define TYPE_FILES 2
typedef struct _torrentmetadata_t{
  unsigned char info_hash[20]; // torrent的info_hash值(info键对应值的SHA1哈希值)
  char *announce; // tracker的URL
  int length;     // 文件总长度
  //char *name;     // 文件名
  int piece_len;  // 每一个分片的字节数
  int num_pieces; // 分片数量
  char *pieces;   // 针对所有分片的20字节长的SHA1哈希值连接而成的字符串
  int file_num;     //文件数目
  file_info *files;     //文件信息
} torrentmetadata_t;

//这个函数解析.torrent文件中announce部分
announce_url_t* parse_announce_url(char* announce);

// 注意: 这个函数可以处理多文件模式torrent
torrentmetadata_t* parsetorrentfile(char* filename);

#endif // PARSETORRENTFILE_H_INCLUDED
