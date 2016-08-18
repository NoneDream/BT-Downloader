/*
 * bencode解码的C语言实现.
 * BitTorrent定义的B编码格式详见实验讲义的附录G和下面的网址:
 *  https://wiki.theory.org/BitTorrentSpecification#Bencoding
 */

/* 使用方法:
 *  - 将B编码数据传递给be_decode()
 *  - 解析返回的结果树
 *  - 调用be_free()释放资源
 */

#ifndef _BENCODE_H
#define _BENCODE_H

typedef enum {
	BE_STR,
	BE_INT,
	BE_LIST,
	BE_DICT,
} be_type;

struct _be_dict;
struct _be_node;

typedef struct _be_dict{
	char *key;//键
	struct _be_node *val;//值
}be_dict;//字典

typedef struct _be_node{
	be_type type;
	union{
		char *s;//字符串
		long long i;//整数
		struct _be_node **l;//列表
		be_dict *d;//字典（指向be_dict的数组）
	}val;
	int len;
}be_node;

///ben_res->val.d[i].val->val.s

long long be_str_len(be_node *node);
be_node *be_decode(const char *bencode);
be_node *be_decoden(const char *bencode, long long bencode_len);
void be_free(be_node *node);
void be_dump(be_node *node);

#endif
