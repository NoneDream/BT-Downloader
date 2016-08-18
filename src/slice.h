#ifndef PIECE_H_
#define PIECE_H_

#include "btdata.h"

///当一个分片保存完成后，并不马上删除缓存，而是置为SAVED状态，若一段时间内没有请求到来，再删除缓存
#define PIECE_INCOMPLETE 0//未下载
#define PIECE_DOWNLOADING 1//正在下载
#define PIECE_COMPLETE 2//下载完成待保存
#define PIECE_SAVED 3//已保存

//每个分片的信息
typedef struct _piece_t{
    int index;//分片基于0的索引
	int state;//状态
	char *field;//记录子分片状态
	char * data;//数据缓存
	int buffered_len;
	time_t lastRequested;
}piece_t;

//待保存分片的链表
typedef struct _piece_chain{
    piece_t *piece;
    struct _piece_chain *next;
}piece_chain;

//一个待保存的分片
/*typedef struct _piece_chain{
    int index;//分片基于0的索引
    char *data;//数据缓存（不要忘记释放！）
    piece_chain *next;//下一个子分片
}piece_chain;*/

/*typedef struct _piecePool_t{
	uint32_t fileLen;
	int fs;
	uint32_t pieceLen;
	uint32_t pieceNum;
	uint8_t *pieceHash;

	uint8_t *pieceCompletedBitField;
	uint8_t *pieceDownloadingBitField;
	uint8_t *pieceProvideBitField;

	pthread_mutex_t mutex;
}piecePool_t;*/

//这个函数初始化bitfield
void piecefield_init(void);

//这个线程负责将数据包保存到分片的缓存中
void *save_to_buf(void *arg);

//这个函数将在一个分片下载完成后执行，负责修改分片状态和bitfield，将分片加入待保存队列，并向peer发送have消息，重置生存时间
void have(int piece_num);

//这个线程负责监视分片状态
//若分片状态为SAVED且RELEASE_TIME秒内没有peer请求此分片，则删除缓存
void *release_piece_buf(void *arg);

#endif
