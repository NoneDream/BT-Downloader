#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "files.h"
#include "btdata.h"
#include "global.h"
#define __USE_GNU
#include <errno.h>

///SHA1(const unsigned char *d, unsigned long n, unsigned char *md);
///参数d指向需要计算哈希值的数据, n是数据的长度(以字节为单位), md指向计算好的哈希值. 你需要为md分配空间, 并且它至少能
///保存SHA_DIGEST_LENGTH=20个字节. 更详细的信息参见"man SHA1".
///在编译时, 需要添加"-lcrypto"选项来链接openssl.

//g_uploaded
//g_downloaded
//g_left
//g_complete_num
//g_incomplete_num
//g_bitfield
int file_check(task_info_struct *task_info)//这个函数检查所有文件是否存在，若不存在则创建，初始化文件信息，并检查分片完成情况
{
	int piece_n=0;
	//int offset=0;
	int len_cnt=0;
	file_info *p;
	char *sha1_p=g_torrentdata->pieces;
	char md[20];
	char *buf;
	//unsigned char *md_2;

	///设置当前目录为下载目录。其实，就只要加入这一句，就可以使用相对路径打开文件
	if(task_info->downlocation[0]=='/')chdir(task_info->downlocation);
	else{
        char cwd[128];
        char *cwdd=getcwd(cwd,120);
        char *dnloc=task_info->downlocation;
        if(task_info->downlocation[0]=='.')
            dnloc+=2;
        if(cwdd == NULL){
            printf("[Error][file_check]CWD is too long!");
        }
        cwdd=(char *)malloc(strlen(cwd)+strlen(dnloc)+2);
        strcpy(cwdd,cwd);
        strcpy(cwdd+strlen(cwd)+1,dnloc);
        cwdd[strlen(cwd)]='/';
        cwdd[strlen(cwd)+strlen(dnloc)+1]=0;
        chdir(cwdd);
	}


	p = task_info->torrentdata->files;

    while(p->path!=NULL){
        if(access(p->path,F_OK)){
            ///建立不存在的文件
            printf("File '%s' doesn't exist, create it!\n",p->path);
            p->f=fopen(p->path,"w+");
            if(p->f==NULL){
                perror("[Error][file_check]File creat failed!");
                exit(250);
            }
            fclose(p->f);
            truncate(p->path,p->length);
            p->new_flag=1;
        }
        else {
            if(access(p->path, R_OK | W_OK)){
                printf("[Error][file_check]Can't read or write file '%s', exit!\n",p->path);
                exit(233);
            }
            else if(p->length!=file_size(p->path)){
                ///修改大小不一致的文件
                printf("[Warning][file_check]File '%s' does exist but size is wrong,resize.\n",p->path);
                truncate(p->path,p->length);
                p->new_flag=1;
            }
            else{
                printf("File '%s' exists,pass.\n",p->path);
                p->new_flag=0;
            }
        }
        p->f=fopen(p->path,"r+");
        p->piece_start=(int)floor(len_cnt/g_torrentdata->piece_len);
        p->piece_offset=len_cnt-(p->piece_start*g_torrentdata->piece_len);
        len_cnt+=p->length;
        p->piece_end=(int)ceil((double)len_cnt/g_torrentdata->piece_len)-1;
        pthread_mutex_init(&p->file_mutex,NULL);
        ++p;
    }

    ///遍历所有文件，确认完成和未完成的片
    printf("Start check.....\n");
    p=g_torrentdata->files;
    buf=malloc(g_torrentdata->piece_len);
    for(piece_n=0;piece_n<g_torrentdata->num_pieces;++piece_n){
        printf("Piece %d\t",piece_n);
        ///===========调试用==============
        if(piece_n==100){
            printf("[Debug]Break point.\n");
        }
        ///===============================
        if(p->new_flag==1){
            ///跳过新建的文件
            piece_n=p->piece_end;
            ++p;
            printf("New file,passed.\n");
            continue;
        }
        if(get_piece(buf,&p,piece_n,0)){
            if(piece_n<g_torrentdata->num_pieces)SHA1((unsigned char *)buf, g_torrentdata->piece_len, (unsigned char *)md);
            else SHA1((unsigned char *)buf, g_torrentdata->length%g_torrentdata->piece_len, (unsigned char *)md);
            if(0==strncmp(md,sha1_p,20)){
                g_piecefield[piece_n].state=PIECE_SAVED;
                ++g_complete_num;
                --g_incomplete_num;
                printf("complete\n");
            }
            else{
                printf("incomplete\n");
            }
        }
        else{
            printf("Fail to read,pass.[Waring]\n");
        }
        sha1_p+=20;
    }
    printf("Check complete,%d pieces in all,%d complete,%d left.\n",g_torrentdata->num_pieces,g_complete_num,g_incomplete_num);
    ///----------------------------------------------------------------
    free(buf);

    return 0;
}

//这个函数返回文件大小
int file_size(char* filename)
{
    struct stat statbuf;
    stat(filename,&statbuf);
    int size=statbuf.st_size;

    return size;
}

// 计算一个打开文件的字节数
int file_len(FILE* fp)
{
  int sz;
  fseek(fp , 0 , SEEK_END);
  sz = ftell(fp);
  rewind (fp);
  return sz;
}

//这个函数用来获取一个分片的数据，输入指针为分片起始位置的文件的指针，成功返回1，失败返回0
int get_piece(char *data,file_info **pp,int piece_num,int len)///若输入len为0，则函数自动判断长度
{
    int cnt;

    if(piece_num>=g_torrentdata->num_pieces){
        printf("[Error][get_piece]Piece_num out of value!\n");
        return 0;
    }
    if((**pp).piece_start>piece_num){
        printf("[Error][get_piece]Piece_start>piece_num!\n");
        return 0;
    }

    if(len==0){
        if(piece_num<(g_torrentdata->num_pieces-1))len=g_torrentdata->piece_len;
        else len=g_torrentdata->length%g_torrentdata->piece_len;
    }

    while((*pp)->piece_end<piece_num)++(*pp);

    ///文件操作临界区开始
    pthread_mutex_lock(&(**pp).file_mutex);
    if((**pp).piece_start!=piece_num)fseek((*pp)->f,(piece_num-(**pp).piece_start)*g_torrentdata->piece_len-(**pp).piece_offset,SEEK_SET);
    else fseek((*pp)->f,0,SEEK_SET);
    cnt=fread(data, 1,len , (*pp)->f);
    pthread_mutex_unlock(&(**pp).file_mutex);
    ///文件操作临界区结束
    if(cnt<len){
        if(feof((*pp)->f)){
            ++(*pp);
            if(get_piece(data+cnt,pp,piece_num,len-cnt))return 1;
            else return 0;
        }
        else{
            printf("[Error][get_piece]Can't read enough data(not reach file end)!\n");
            return 0;
        }
    }

    return 1;
}

//这个函数用来保存一个分片的数据，输入指针为分片起始位置的文件的指针，成功返回1，失败返回0
int save_piece(char *data,file_info **pp,int piece_num,int len)///若输入len为0，则函数自动判断长度
{
    int cnt;
    int write_num;
    int position;

    if(piece_num>=g_torrentdata->num_pieces){
        printf("[Error][save_piece]Piece_num out of value!\n");
        return 0;
    }
    if((**pp).piece_start>piece_num){
        printf("[Error][save_piece]Piece_start>piece_num!\n");
        return 0;
    }

    if(len==0){
        if(piece_num<(g_torrentdata->num_pieces-1))len=g_torrentdata->piece_len;
        else len=g_torrentdata->length%g_torrentdata->piece_len;
    }

    while((**pp).piece_end<piece_num)++(*pp);

    if((**pp).piece_start!=piece_num)position=(piece_num-(**pp).piece_start)*g_torrentdata->piece_len-(**pp).piece_offset;
    else position=0;
    if(len>((**pp).length-position))write_num=len-((**pp).length-position);
    else write_num=len;
    ///文件操作临界区开始
    pthread_mutex_lock(&(**pp).file_mutex);
    fseek((**pp).f,position,SEEK_SET);
    cnt=fwrite(data, 1,write_num , (**pp).f);
    pthread_mutex_unlock(&(**pp).file_mutex);
    ///文件操作临界区结束
    if(cnt<write_num){
        printf("[Error][save_piece]Can't write enough data(not reach file end)!\n");
        return 0;
    }
    if(write_num<len){
        if(save_piece(data+write_num,pp,piece_num,len-write_num))return 1;
        else return 0;
    }

    return 1;
}

//这个线程负责将下载完成的分片保存至磁盘文件,若检测到链表为空，则结束进程
void *save_to_disk(void *arg)
{
    int piece_num;
    file_info *tmp;
    int flag_error;

    while(1){
        sem_wait(piece_num_to_save);
        if(piecetosave_head==NULL)break;
        piece_num=piecetosave_head->piece->index;
        tmp=g_torrentdata->files;
        flag_error=0;
        while(tmp->piece_end<piece_num){
            ++tmp;
            if(tmp->path==NULL){
                printf("[Error][save_to_disk]Piece index out of value!\n");
                flag_error=1;
            }
        }
        if(flag_error)continue;
        save_piece(piecetosave_head->piece->data,&tmp,piece_num,0);
        piecetosave_head->piece->state=PIECE_SAVED;
        piecetosave_head=piecetosave_head->next;
    }

    printf("Thread 'save_to_disk' closed successfully.\n");
    return (void *)0;
}
