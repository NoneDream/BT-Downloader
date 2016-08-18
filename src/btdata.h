#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <semaphore.h>

#ifndef BTDATA_H
#define BTDATA_H

/**************************************
 * 一些常量定义
**************************************/
///////////////////////////////////////////////////////////////////////////////////
#define HANDSHAKE_LEN 68  // peer握手消息的长度, 以字节为单位
#define BT_PROTOCOL "BitTorrent protocol"
#define INFOHASH_LEN 20
#define PEER_ID_LEN 20
#define MAXPEERS 100
#define KEEP_ALIVE_INTERVAL 100///keepalive的发送间隔，单位秒
#define PEER_HANDSHAKE_TIMEOUT 20///判断peer握手超时的时间

#define PEER_TIME_OUT 150///单位：秒
#define SUB_SLICE_NUM 4

#define RELEASE_TIME 30///分片缓存的生存时间
#define SLICE_LEN 16384 ///子分片长度（16KB）
///////////////////////////////////////////////////////////////////////////////////

#define BT_STARTED 0
#define BT_STOPPED 1
#define BT_COMPLETED 2

///位域bitfield
typedef struct{
    unsigned char bit0:1;
    unsigned char bit1:1;
    unsigned char bit2:1;
    unsigned char bit3:1;
    unsigned char bit4:1;
    unsigned char bit5:1;
    unsigned char bit6:1;
    unsigned char bit7:1;
}byte;

#endif
