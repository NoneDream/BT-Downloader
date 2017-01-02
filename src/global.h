#ifndef __GLOBAL__
#define __GLOBAL__

#include "parsetorrentfile.h"
#include "peer_wire.h"
#include "slice.h"

/**************************************
全局结构体
**************************************/

///本机信息
typedef struct _host_info_struct{
    int peerport; // 作为peer监听的端口号
    char my_ip[20]; // 格式为XXX.XXX.XXX.XXX, null终止
    char my_id[21];//本机的peer_id
}host_info_struct;

///任务信息
typedef struct _task_info_struct{
    struct sockaddr_in tracker_addr;//tracker的地址
    bitmap *piecefield;
    torrentmetadata_t *torrentdata;//torrent文件中的数据
    int isseed;//本机是种子还是下载者4
    int complete_num;//已完成分片数
    int incomplete_num;//待完成分片数
    int uploaded;//已经上传的字节数
    int downloaded;//已经下载的字节数
    int left;//还需下载的字节数
    ///int tracker_port;//tracker的端口
    ///in_addr_t tracker_ip; // tracker的IP地址
    pthread_mutex_t peer_list_mutex;
    peer_t *peer_list[MAXPEERS];//peer信息
    char downlocation[128];//下载文件路径
}task_info_struct;

///跨线程的信号量与工作队列
peer_packet_chain *pkttosave_head;//待保存数据包链表头
peer_packet_chain *pkttosave_end;//待保存数据包链表尾
sem_t packet_num_to_save;//信号量，指示还有多少数据包需要保存
piece_chain *piecetosave_head;//待保存分片的链表头
piece_chain *piecetosave_end;//待保存分片的链表尾
sem_t piece_num_to_save;//信号量，指示还有多少分片需要保存

#endif
