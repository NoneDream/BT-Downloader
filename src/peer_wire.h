#ifndef PEER_WIRE_H
#define PEER_WIRE_H

#include <pthread.h>
#include "slice.h"
#include "tracker_wire.h"

#define LISTENING_PORT 6888

#define PSTR "BitTorrent protocol"

#define CHOKE_MSG_ID 0x00
#define UNCHOKE_MSG_ID 0X01
#define INTERESTED_MSG_ID 0X02
#define NOTINTERESTED_MSG_ID 0X03
#define HAVE_MSG_ID 0X04
#define BITFIELD_MSG_ID 0x05
#define REQUEST_MSG_ID 0X06
#define PIECE_MSG_ID 0X07
#define CANCLE_MSG_ID 0X08

// 针对到一个peer的已建立连接, 维护相关数据
#define STATE_ENABLE 0
#define STATE_DISABLE 1
typedef struct _peer_t {
  int sockfd;
  int choking;        // 作为上传者, 阻塞远端peer
  int interested;     // 远端peer对我们的分片有兴趣
  int choked;         // 作为下载者, 我们被远端peer阻塞
  int have_interest;  // 作为下载者, 对远端peer的分片有兴趣
  char peer_id[21];
  uint8_t *bitField;  //Slice provided by another peer
  pthread_t thread_keepalive;
  //pthread_mutex_t mutex;
  time_t lastAlive;
} peer_t;

#pragma pack(1)

typedef struct
{
	uint32_t len;
	uint8_t msgID;
	unsigned char *data;
}peer_wire_packet;

typedef struct{
    uint32_t index;
    uint32_t begin;
    unsigned char block[SLICE_LEN];
}piece_packet;

typedef struct{
    uint8_t pstrlen;
	unsigned char pstr[19];
	uint8_t reserved[8];
	unsigned char info_hash[20];
	unsigned char peer_id[20];
}packet_handshake;

typedef struct{
    uint32_t len;
	uint8_t msgID;
	uint32_t piece_index;
}packet_have;

#pragma pack()
struct _peer_packet_chain;
typedef struct _peer_packet_chain{
    peer_wire_packet *packet;
    struct _peer_packet_chain *next;
}peer_packet_chain;

#include "btdata.h"

 //从peer_pool得到一个槽，默认peer_pool的大小是MAXPEERS，所以请保证实际peer_pool的大小大于MAXPEERS，不同的peer用socketfd来区分，若找不到sock制定的peer，则返回NULL
peer_t* get_slot(int sock, peer_t **peer_pool);

 //从peer_pool得到一个槽，默认peer_pool的大小是MAXPEERS，所以请保证实际peer_pool的大小大于MAXPEERS，不同的peer用socketfd来区分，若找不到peer_id制定的peer，则新建一个
peer_t* get_slot_byid(char *peer_id, peer_t **peer_pool);

//这个函数删除sock指定的槽
int del_slot(int sock, peer_t **peer_pool);

//尝试从另一个peer获取一个数据包（peer_wire_packet），成功返回0，失败返回-1
int recv_peerWireMsg(int sock, peer_wire_packet *pPacket);

//这个函数用来释放一个peer_wire_packet结构
int free_peerWireMsg(peer_wire_packet *pPacket);

//这个线程负责发送keepalive信号，输入为peer_pool的一个槽
void *keep_alive(void *arg);

//这个线程负责与一个peer建立连接（握手成功后给peer分配一个槽，再返回）
void *connect_to_peer(void *arg);

//这个函数负责根据tracker返回的列表与peer建立链接
void connect_to_peers(tracker_data *peer_data);

//这个线程负责监听所有peer
void *listen_to_peers(void *arg);

/*
int packet_handShake(peer_wire_packet *packet,uint8_t *info_hash, uint8_t* peer_id);
int packet_keepAlive(peer_wire_packet *packet);
int packet_choke(peer_wire_packet *packet);
int packet_unchoke(peer_wire_packet *packet);
int packet_interested(peer_wire_packet *packet);
int packet_notInterested(peer_wire_packet *packet);
int packet_have(peer_wire_packet *packet, uint8_t index[4]);
int packet_bitfield(peer_wire_packet *packet,uint32_t len, uint8_t* bitfield);
int packet_request(peer_wire_packet *packet,uint8_t index[4], uint8_t begin[4], uint8_t length[4]);
int packet_piece(peer_wire_packet *packet,uint8_t index[4], uint8_t begin[4], uint8_t *block);
int packet_cancel(peer_wire_packet *packet,uint8_t index[4], uint8_t begin[4], uint32_t length);

peer_t* get_slot(int sock, peer_t **peer_pool);
int del_slot(int sock, peer_t **peer_pool);

int recv_peerWireMsg(int sock, peer_wire_packet *pPacket);
int free_peerWireMsg(peer_wire_packet *pPacket);
int connect_to_peer(const piecePool_t *pPool,torrentmetadata_t pMeta, peer_t *pPeer);
*/
#endif
