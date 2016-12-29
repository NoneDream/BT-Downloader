#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "slice.h"
#include "global.h"
#include "bitmap.h"

//这个函数初始化bitfield
void piecefield_init(void)
{
    int i;

    g_complete_num=0;
    g_incomplete_num=g_torrentdata->num_pieces;

    g_piecefield=malloc(g_torrentdata->num_pieces*sizeof(piece_t));

    field_size=(int)ceil(g_torrentdata->piece_len/SLICE_LEN);
    for(i=0;i<g_torrentdata->num_pieces;++i){
        g_piecefield[i].state=PIECE_INCOMPLETE;
        g_piecefield[i].index=i;
        g_piecefield[i].buffered_len=0;
        g_piecefield[i].field=malloc(field_size);
        memset(g_piecefield[i].field,0,field_size);
    }
}

//这个线程负责将数据包保存到分片的缓存中,若检测到链表为空，则结束进程
void *save_to_buf(void *arg)
{
    int field_offset;
    piece_packet *data;
    peer_packet_chain *tmp;

    while(1){
        sem_wait(packet_num_to_save);
        if(pkttosave_head==NULL)break;
        if(pkttosave_head->packet->msgID==7){
            data=(piece_packet *)pkttosave_head->packet->data;
            field_offset=(int)data->begin/SLICE_LEN;
            if(0==g_piecefield[data->index].field[field_offset]){
                memcpy(g_piecefield[data->index].data+(field_offset*SLICE_LEN),data->block,pkttosave_head->packet->len-9);
                g_piecefield[data->index].field[field_offset]=1;
                g_piecefield[data->index].buffered_len+=pkttosave_head->packet->len-9;
                ///检查分片完成情况并加入待保存队列
                if(g_torrentdata->piece_len==g_piecefield[data->index].buffered_len)have(data->index);
                else if(data->index==g_torrentdata->num_pieces-1){
                    if(g_piecefield[data->index].buffered_len==g_torrentdata->length%g_torrentdata->piece_len)have(data->index);
                }
            }
        }
        else{
            printf("[Warning][save_to_buf]Wrong message id!\n");
        }
        tmp=pkttosave_head;
        pkttosave_head=pkttosave_head->next;
        ///释放已缓存的包
        free(tmp->packet->data);
        free(tmp->packet);
        free(tmp);
    }

    printf("Thread 'save_to_buf' closed successfully.\n");
    return (void *)0;
}

//这个函数将在一个分片下载完成后执行，负责修改分片状态和bitfield，将分片加入待保存队列，并向peer发送have消息，重置生存时间
void have(int piece_num)
{
    packet_have send_buf;
    int i;

    if(piecetosave_end!=piecetosave_head){
        piecetosave_end->next=malloc(sizeof(piece_chain));
        piecetosave_end=piecetosave_end->next;
    }
    else{
        piecetosave_end=malloc(sizeof(piece_chain));
        piecetosave_head=piecetosave_end;
    }
    piecetosave_end->piece=g_piecefield+piece_num;
    sem_post(piece_num_to_save);

    send_buf.len=htonl(5);
    send_buf.msgID=4;
    send_buf.piece_index=htonl(piece_num);
    for(i=0;i<MAXPEERS;++i){
        if(NULL!=g_peer_pool[i]){
            if(g_peer_pool[i]->sockfd>=0){
                send(g_peer_pool[i]->sockfd,&send_buf,sizeof(packet_have),0);
            }
        }
    }

    g_piecefield[piece_num].lastRequested=time(NULL);

    return;
}

//这个线程负责监视分片状态
//若分片状态为SAVED且RELEASE_TIME秒内没有peer请求此分片，则删除缓存
void *release_piece_buf(void *arg)
{
    time_t now;
    int i;

    while(1){
        if(g_done)break;
        sleep(RELEASE_TIME);
        now=time(NULL)+RELEASE_TIME;
        for(i=0;i<g_torrentdata->num_pieces;++i){
            if(g_piecefield[i].state==PIECE_SAVED){
                if(g_piecefield[i].lastRequested<now){
                    g_piecefield[i].buffered_len=0;
                    free(g_piecefield[i].data);
                }
            }
        }
    }

    printf("Thread 'release_piece_buf' exit.\n");
    return (void *)0;
}
