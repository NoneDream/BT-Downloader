#ifndef __GLOBAL__
#define __GLOBAL__

#include "parsetorrentfile.h"
#include "peer_wire.h"
#include "slice.h"

/**************************************
 * 全局变量
**************************************/

#ifdef DEFINE

///网络参数
char g_my_ip[128]; // 格式为XXX.XXX.XXX.XXX, null终止
char g_my_id[21];//本机的peer_id
int g_peerport; // 作为peer监听的端口号--------------------------------------------------------------------------------
char g_tracker_ip[16]; // tracker的IP地址, 格式为XXX.XXX.XXX.XXX(null终止)
int g_tracker_port;//tracker的端口

///任务属性数据
torrentmetadata_t *g_torrentdata;//torrent文件中的数据
int g_isseed;//本机是种子还是下载者4
char g_downlocation[128];//下载文件路径
//int g_infohash[5];//torrent文件info键的特征

///peer
pthread_mutex_t g_peer_pool_mutex;
peer_t *g_peer_pool[MAXPEERS];

///分片记录（在piecefield_init中初始化）
int field_size;//每个分片bitfield的大小
piece_t *g_piecefield;
int g_complete_num;
int g_incomplete_num;

/// 这些变量用在函数make_tracker_request中, 它们需要在客户端执行过程中不断更新.
int g_uploaded;//已经上传的字节数
int g_downloaded;//已经下载的字节数
int g_left;//还需下载的字节数

///跨线程的信号量与工作队列
peer_packet_chain *pkttosave_head;//待保存数据包链表头
peer_packet_chain *pkttosave_end;//待保存数据包链表尾
sem_t packet_num_to_save;//信号量，指示还有多少数据包需要保存
piece_chain *piecetosave_head;//待保存分片的链表头
piece_chain *piecetosave_end;//待保存分片的链表尾
sem_t piece_num_to_save;//信号量，指示还有多少分片需要保存

int g_done; // 表明程序是否应该终止

#else

///网络参数
extern char g_my_ip[128]; // 格式为XXX.XXX.XXX.XXX, null终止
extern char g_my_id[21];//本机的peer_id
extern int g_peerport; // 作为peer监听的端口号
extern char g_tracker_ip[16]; // tracker的IP地址, 格式为XXX.XXX.XXX.XXX(null终止)
extern int g_tracker_port;//tracker的端口

///任务属性数据
extern torrentmetadata_t *g_torrentdata;//torrent文件中的数据
extern int g_isseed;//本机是种子还是下载者4
extern char g_downlocation[128];//下载文件路径
//extern int g_infohash[5];//torrent文件info键的特征

///peer
extern pthread_mutex_t g_peer_pool_mutex;
extern peer_t *g_peer_pool[MAXPEERS];

///分片记录
extern int field_size;
extern piece_t *g_piecefield;
extern int g_complete_num;
extern int g_incomplete_num;

///这些变量用在函数make_tracker_request中, 它们需要在客户端执行过程中不断更新.
extern int g_uploaded;//已经上传的字节数
extern int g_downloaded;//已经下载的字节数
extern int g_left;//还需下载的字节数

///跨线程的信号量与工作队列
extern peer_packet_chain *pkttosave_head;//待保存数据包链表头
extern peer_packet_chain *pkttosave_end;//待保存数据包链表尾
extern sem_t *packet_num_to_save;//信号量，指示还有多少数据包需要保存
extern piece_chain *piecetosave_head;//待保存分片的链表头
extern piece_chain *piecetosave_end;//待保存分片的链表尾
extern sem_t *piece_num_to_save;//信号量，指示还有多少分片需要保存

extern int g_done; // 表明程序是否应该终止

#endif

#endif
