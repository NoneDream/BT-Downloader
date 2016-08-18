#include <pthread.h>
#include <stdint.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "btdata.h"
#include "peer_wire.h"
#include "global.h"
#include "util.h"

/*
 * Get a slot from peer_pool
 * The default size of peer_pool are MAXPEERS
 * So ENSURE that the size of peer_pool are big than MAXPEERS
 * Different peer are identified by sockfd, this function try to find the slot which sockfd same as paramater, otherwhis it will create a new slot.
 * So you can use a invalid sock (-1) to get a new slot
 */
 //从peer_pool得到一个槽，默认peer_pool的大小是MAXPEERS，所以请保证实际peer_pool的大小大于MAXPEERS，不同的peer用socketfd来区分，若找不到sock制定的peer，则会新建一个
peer_t* get_slot(int sock, peer_t **peer_pool)
{
	int i;

	for(i=0;i<MAXPEERS;i++){
        if(peer_pool[i]->sockfd==sock){
                return peer_pool[i];
        }
	}
	if(i==MAXPEERS){
        printf("[Warning][get_slot]Can't find the slot!");
        return NULL;
	}

	return NULL;
}

 //从peer_pool得到一个槽，默认peer_pool的大小是MAXPEERS，所以请保证实际peer_pool的大小大于MAXPEERS，不同的peer用peer_id来区分，若找不到peer_id制定的peer，则会新建一个
peer_t* get_slot_byid(char *peer_id, peer_t **peer_pool)
{
	int i;
	int slotP;

	slotP=-1;

	for(i=0;i<MAXPEERS;i++){
		if(peer_pool[i]==NULL){
			///slotP是第一个非空的槽
			if(slotP==-1){
					slotP=i;
			}
		}
		else{
			if(0==strcmp(peer_pool[i]->peer_id,peer_id)){
					return peer_pool[i];
			}
		}
	}
	if(i==MAXPEERS&&slotP==-1){
        printf("[Warning][get_slot_byid]Can't find an empty slot!");
        return NULL;
	}

	peer_pool[slotP]=(peer_t*)malloc(sizeof(peer_t));
	peer_pool[slotP]->sockfd=-1;
	return peer_pool[slotP];
}

//这个函数删除sock指定的槽
int del_slot(int sock, peer_t **peer_pool)
{
	int i=0;
	for(i=0;i<MAXPEERS;i++){
		if(peer_pool[i]!=NULL){
			if(peer_pool[i]->sockfd==sock){
                pthread_cancel(peer_pool[i]->thread_keepalive);
                printf("[del_slot]NUM:%d send cancel.....\n",i);
                if(sock>0)close(sock);
                peer_pool[i]->sockfd=-1;
                pthread_join(peer_pool[i]->thread_keepalive,NULL);
                printf("[del_slot]NUM:%d joined successfully!\n",i);
                free(peer_pool[i]->bitField);
				free(peer_pool[i]);
				return 0;
			}
		}
	}

	printf("[Warning][del_slot]Can't find the slot by given sock!\n");
	return -1;
}

//尝试从另一个peer获取一个数据包（peer_wire_packet），成功返回0，失败返回-1
int recv_peerWireMsg(int sock, peer_wire_packet *pPacket)
{
    //接收len
    if(recv( sock, (void*)pPacket, 4, MSG_WAITALL) < 4){
        printf("[Error][recv_peerWireMsg(1)]Can't recv enough data from sock:%d!\n",sock);
        return -1;
    }
    else{
        pPacket->len = ntohl(pPacket->len);//网络字节序转换为主机字节序
        if(pPacket->len == 0){
            return 0;
        }
        //接收MsgID
        if(recv( sock, (void*)&pPacket->msgID, 1, MSG_WAITALL) < 1){
            printf("[Error][recv_peerWireMsg(2)]Can't recv enough data from sock:%d!\n",sock);
            return -1;
        }

        pPacket->data = (unsigned char*) realloc(pPacket->data,pPacket->len-1);
        //接收data
        if(recv(sock, (void*)pPacket->data, pPacket->len-1, MSG_WAITALL) < pPacket->len-1){
            printf("[Error][recv_peerWireMsg(3)]Can't recv enough data from sock:%d!\n",sock);
            return -1;
        }
        else{
            return 0;
        }
    }
}

//这个函数用来释放一个peer_wire_packet结构
int free_peerWireMsg(peer_wire_packet *pPacket)
{
	 free(pPacket->data);
	 return 0;
}

//这个线程负责发送keepalive信号，输入为peer_pool的一个槽
void *keep_alive(void *arg)
{
    peer_t *in=(peer_t *)arg;
    uint32_t msg=0;
    int interval_time=KEEP_ALIVE_INTERVAL;

    while(!g_done){
        if(in->sockfd<0)break;
        send(in->sockfd,&msg,sizeof(msg),0);
        sleep(interval_time);
    }

    return (void *)0;
}

//这个线程负责与一个peer建立连接（握手成功后给peer分配一个槽，再返回）
void *connect_to_peer(void *arg)
{
    int sock;
    peer_t *slot;
    peerdata *peer=arg;
    packet_handshake buf;
    fd_set fds;
    struct timeval wait_time;

    pthread_detach(pthread_self());///设置unjoinable属性

    sock=connect_to_host(peer->ip, peer->port);
    if(sock==-1){
        printf("[Warning][connect_to_peer]Connect failure!\n");
        return (void *)-1;
    }

    buf.pstrlen=strlen(PSTR);
    memcpy(buf.pstr,PSTR,buf.pstrlen);
    memset(buf.reserved,0,8);
    memcpy(buf.info_hash,g_torrentdata->info_hash,20);
    memcpy(buf.peer_id,g_my_id,strlen(g_my_id));

    send(sock,&buf,sizeof(buf),0);

    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    wait_time.tv_usec=0;
    wait_time.tv_sec = PEER_HANDSHAKE_TIMEOUT;

    if(select(sock+1, &fds, NULL, NULL, &wait_time)>0){
        if(sizeof(buf)==recv(sock,&buf,sizeof(buf),0)){
            if(0==strcmp((char *)buf.info_hash,(char *)g_torrentdata->info_hash)){
                if(0==strcmp((char *)buf.peer_id,peer->id)){
                    if(0==strcmp((char *)buf.pstr,PSTR)){
                        slot=get_slot(sock,g_peer_pool);
                        if(slot->sockfd>0){
                            printf("[Error][connect_to_peer]Reuse same socket!\n");
                            return (void *)0;
                        }
                        if(slot!=NULL){
                            ///给这个peer分配一个槽，并初始化
                            slot->bitField=malloc(g_torrentdata->num_pieces);
                            memset(slot->bitField,0,g_torrentdata->num_pieces);
                            memset(slot,0,g_torrentdata->num_pieces);
                            memcpy(slot->peer_id,peer->id,21);
                            pthread_create(&slot->thread_keepalive,NULL,&keep_alive,(void *)slot);
                            slot->lastAlive=time(NULL);
                            slot->sockfd=sock;
                            printf("Successfully handshake with peer id:'%s'.\n",peer->id);
                            return (void *)0;
                        }
                        else{
                            printf("[Warning][connect_to_peer]Don't have enough slot!\n");
                        }
                    }
                    else{
                        printf("[Warning][connect_to_peer]Invalid PSTR!\n");
                    }
                }
                else{
                    printf("[Warning][connect_to_peer]Invalid peer_id!\n");
                }
            }
            else{
                printf("[Warning][connect_to_peer]Invalid info_hash!\n");
            }
        }
        else{
            printf("[Warning][connect_to_peer]Recv handshake packet with wrong length!\n");
        }
    }
    else{
        printf("[Warning][connect_to_peer]Peer timeout!\n");
    }

    return (void *)-1;
}

//这个函数负责根据tracker返回的列表与peer建立链接
void connect_to_peers(tracker_data *peer_data)
{
    int i;
    peer_t *tmp;
    pthread_t nouse;

    for(i=0;i<peer_data->numpeers;++i){
        tmp=get_slot_byid(peer_data->peers[i].id,g_peer_pool);
        if(tmp==NULL){
            printf("[Warning][connect_to_peers]Don't have enough slot!\n");
            return;
        }
        if(tmp->sockfd==-1){
            if(0!=pthread_create(&nouse,NULL,&connect_to_peer,peer_data->peers+i)){
                printf("[Warning][connect_to_peers]Creat thread 'connect_to_peer' failed!num:%d\n",i);
            }
        }
        else{
            printf("Peer '%s' already connected,jumped.\n",peer_data->peers[i].id);
        }
    }

    return;
}

///这个函数负责刷新bitfield
void bitfield_refresh(peer_t *pPeer,peer_wire_packet *packetBuf)
{
    uint8_t *fie=pPeer->bitField;
    byte *tmp=(byte *)packetBuf->data;
    int k=(int)ceil(g_torrentdata->num_pieces/8);
    int left=g_torrentdata->num_pieces%8;
    int cnt;

    if((packetBuf->len-1)!=k){
        printf("[Error][bitfield_refresh]Recived bitfield of wrong length,kick this peer.\n");
        del_slot(pPeer->sockfd,g_peer_pool);
        return;
    }

    if(left!=0)--k;
    for(cnt=0;cnt<k;++cnt){
        if(tmp->bit0)*fie=1;
        ++fie;
        if(tmp->bit1)*fie=1;
        ++fie;
        if(tmp->bit2)*fie=1;
        ++fie;
        if(tmp->bit3)*fie=1;
        ++fie;
        if(tmp->bit4)*fie=1;
        ++fie;
        if(tmp->bit5)*fie=1;
        ++fie;
        if(tmp->bit6)*fie=1;
        ++fie;
        if(tmp->bit7)*fie=1;
        ++fie;
        ++tmp;
    }
    if(left==0)return;
    if(tmp->bit0)*fie=1;
    ++fie;
    --left;
    if(left==0)return;
    if(tmp->bit1)*fie=1;
    ++fie;
    --left;
    if(left==0)return;
    if(tmp->bit2)*fie=1;
    ++fie;
    --left;
    if(left==0)return;
    if(tmp->bit3)*fie=1;
    ++fie;
    --left;
    if(left==0)return;
    if(tmp->bit4)*fie=1;
    ++fie;
    --left;
    if(left==0)return;
    if(tmp->bit5)*fie=1;
    ++fie;
    --left;
    if(left==0)return;
    if(tmp->bit6)*fie=1;
    ++fie;
    --left;
    if(left==0)return;
    if(tmp->bit7)*fie=1;
    ++fie;
    --left;

    printf("[Error][bitfield_refresh]Out of band position!program wrong,please check!\n");

    return;
}

///这个函数负责将packet加入待缓存队列
void addto_buffer_chain(peer_wire_packet *packetBuf)
{
    if(pkttosave_end!=pkttosave_head){
        pkttosave_end->next=malloc(sizeof(peer_packet_chain));
        pkttosave_end=pkttosave_end->next;
    }
    else{
        pkttosave_end=malloc(sizeof(peer_packet_chain));
        pkttosave_head=pkttosave_end;
    }
    pkttosave_end->packet=packetBuf;
    sem_post(packet_num_to_save);
}

//这个线程负责监听所有peer-------------------------------------------------------------------------------------数据保存部分待完成
void *listen_to_peers(void *arg)
{
    int i,max;
	peer_wire_packet *packetBuf;
	fd_set fds;
	peer_t *pPeer;

	///pthread_detach(pthread_self());///设置unjoinable属性
	packetBuf=malloc(sizeof(peer_wire_packet));

	while(!g_done){
        ///确定所有要监视的socket
        FD_ZERO(&fds);
        max=0;
        for(i=0;i<MAXPEERS;++i){
            if(g_peer_pool[i]!=NULL){
                if(g_peer_pool[i]->sockfd>=0){
                    FD_SET(g_peer_pool[i]->sockfd, &fds);
                    if(g_peer_pool[i]->sockfd>max)max=g_peer_pool[i]->sockfd;
                }
            }
        }

        if(select(max+1, &fds, NULL, NULL, NULL)<=0)continue;
        for(i=0;i<MAXPEERS;++i){
            ///遍历所有socket，接收数据
            pPeer=g_peer_pool[i];
            if(pPeer==NULL)continue;
            if(pPeer->sockfd<0)continue;
            if(0==FD_ISSET(pPeer->sockfd, &fds))continue;
            if(recv_peerWireMsg(pPeer->sockfd, packetBuf)){
                printf("[Warning][listen_to_peers]Fail to recevie packet from peer '%s'!\n",pPeer->peer_id);
                continue;
            }
            else{
                switch(packetBuf->msgID){
                    case CHOKE_MSG_ID:{
                        pPeer->choked = STATE_ENABLE;
                        break;
                    }
                    case UNCHOKE_MSG_ID:{
                        pPeer->choked =STATE_DISABLE;
                        break;
                    }
                    case INTERESTED_MSG_ID:{
                        pPeer->interested = STATE_ENABLE;
                        break;
                    }
                    case NOTINTERESTED_MSG_ID:{
						pPeer->interested = STATE_DISABLE;
						break;
                    }
                    case HAVE_MSG_ID:{
                        ///向peer的bitfield添加
                        pPeer->bitField[ntohl(*(uint32_t *)packetBuf->data)]=1;
						break;
                    }
                    case BITFIELD_MSG_ID:{
                        ///刷新peer的bitfield
                        bitfield_refresh(pPeer,packetBuf);
						break;
                    }
                    case REQUEST_MSG_ID:{
						break;
                    }
                    case PIECE_MSG_ID:{
                        ///将packetBuf加入待缓存队列，并为指针重新分配空间
                        addto_buffer_chain(packetBuf);
                        packetBuf=malloc(sizeof(peer_wire_packet));
                        break;
                    }
                    case CANCLE_MSG_ID:{
                        ///暂不作响应
						break;
                    }
                }
            }
        }
    }

    printf("Thread 'listen_to_peers' returned normally.\n");
	return (void *)0;
}
