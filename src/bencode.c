/*
 * bencode解码的C语言实现.
 * BitTorrent定义的B编码格式详见实验讲义的附录G和下面的网址:
 *  https://wiki.theory.org/BitTorrentSpecification#Bencoding
 *
 * 查看头文件bencode.h以了解使用方法.
 */

#include <stdlib.h> /* malloc() realloc() free() strtoll() */
#include <string.h> /* memset() */

#include "bencode.h"

//这个函数初始化一个be_node结构
static be_node *be_alloc(be_type type)
{
	be_node *ret = malloc(sizeof(be_node));
	if (ret) {
		memset(ret, 0x00, sizeof(be_node));
		ret->type = type;
	}
	return ret;
}

//这个函数解析data头部用字符串表示的十进制数值，并将指针做相应后移
static long long _be_decode_int(const char **data, long long *data_len)
{
	char *endp;
	long long ret = strtoll(*data, &endp, 10);
	*data_len -= (endp - *data);
	*data = endp;
	return ret;
}

//这个函数返回一个字符串型node的长度（保存在s指针之前）
long long be_str_len(be_node *node)
{
	long long ret = 0;
	if (node->val.s)
		memcpy(&ret, node->val.s - sizeof(ret), sizeof(ret));
	return ret;
}

//这个函数解析一个字符串（返回字符串之前还有sizeof(long long)的空间保存了此字符串的长度）
static char *_be_decode_str(const char **data, long long *data_len)
{
	long long sllen = _be_decode_int(data, data_len);
	long slen = sllen;
	unsigned long len;
	char *ret = NULL;

	if (sllen < 0)
		return ret;

	/* 拒绝分配一个超过malloc()使用的size_t类型允许值范围的大值 */
	if (sizeof(long long) != sizeof(long))
		if (sllen != slen)
			return ret;

	/* 确认我们有足够的数据 */
	if (sllen > *data_len - 1)
		return ret;

	/* 从signed类型切换到unsigned类型 */
	len = slen;

	if (**data == ':') {
        /*_ret不知道为何用,然而并不会造成内存溢出，但是释放内存时要将ret前移回头部*///////////
		char *_ret = malloc(sizeof(long long) + len + 1);
		memcpy(_ret, &sllen, sizeof(long long));
		//////////////////////////////////////////////////////////////////////////////////////////////////////////
		ret = _ret + sizeof(long long);
		memcpy(ret, *data + 1, len);
		ret[len] = '\0';
		*data += len + 1;
		*data_len -= len + 1;
	}
	return ret;
}

//这个函数负责解析一个node并将data指针移到下一个node，采用递归结构来解析全部数据
static be_node *_be_decode(const char **data, long long *data_len)
{
	be_node *ret = NULL;

	if (!*data_len)
		return ret;

	switch (**data) {
		/* 列表 */
		case 'l': {
			unsigned int i = 0;

			ret = be_alloc(BE_LIST);

			--(*data_len);
			++(*data);
			while (**data != 'e') {
				ret->val.l = realloc(ret->val.l, (i + 2) * sizeof(be_node *));///i+2是因为最后有一个NULL指针指示列表结束
				ret->val.l[i] = _be_decode(data, data_len);
				++i;
			}
			--(*data_len);
			++(*data);

			ret->val.l[i] = NULL;

			return ret;
		}

		/* 字典 */
		case 'd': {
			unsigned int i = 0;

			ret = be_alloc(BE_DICT);

			--(*data_len);
			++(*data);
			while (**data != 'e') {
				ret->val.d = realloc(ret->val.d, (i + 2) * sizeof(*ret->val.d));
				ret->val.d[i].key = _be_decode_str(data, data_len);
				ret->val.d[i].val = _be_decode(data, data_len);
				++i;
			}
			--(*data_len);
			++(*data);

			ret->val.d[i].val = NULL;
			ret->val.d[i].key=NULL;

			return ret;
		}

		/* 整数 */
		case 'i': {
			ret = be_alloc(BE_INT);

			--(*data_len);
			++(*data);
			ret->val.i = _be_decode_int(data, data_len);
			if (**data != 'e'){
                be_free(ret);
                return NULL;
			}
			--(*data_len);
			++(*data);

			return ret;
		}

		/* 字符串 */
		case '0'...'9': {
			ret = be_alloc(BE_STR);
			ret->len=atoi(*data);
			ret->val.s = _be_decode_str(data, data_len);

			return ret;
		}

		/* invalid */
		default:break;
	}

	return ret;
}

//对_be_decode的封装
be_node *be_decoden(const char *data, long long len)
{
	return _be_decode(&data, &len);
}

//对_be_decode的第二层封装，由于b编码中不会出现/0，故长度不需输入
be_node *be_decode(const char *data)
{
	return be_decoden(data, strlen(data));
}

//这个函数释放前方带有 long long 类型长度的字符串
static inline void _be_free_str(char *str)
{
	if (str)
		free(str - sizeof(long long));
}

//这个函数递归释放be_node占用的全部空间
void be_free(be_node *node)
{
	switch (node->type) {
		case BE_STR:
			_be_free_str(node->val.s);
			break;

		case BE_INT:
			break;

		case BE_LIST: {
			unsigned int i;
			for (i = 0; node->val.l[i]; ++i)
				be_free(node->val.l[i]);
			free(node->val.l);
			break;
		}

		case BE_DICT: {
			unsigned int i;
			for (i = 0; node->val.d[i].val; ++i) {
				_be_free_str(node->val.d[i].key);
				be_free(node->val.d[i].val);
			}
			free(node->val.d);
			break;
		}
	}
	free(node);
}

#ifdef BE_DEBUG
#include <stdio.h>
#include <stdint.h>

static void _be_dump_indent(ssize_t indent)
{
	while (indent-- > 0)
		printf("    ");
}
static void _be_dump(be_node *node, ssize_t indent)
{
	size_t i;

	_be_dump_indent(indent);
	indent = abs(indent);

	switch (node->type) {
		case BE_STR:
			printf("str = %s (len = %lli)\n", node->val.s, be_str_len(node));
			break;

		case BE_INT:
			printf("int = %lli\n", node->val.i);
			break;

		case BE_LIST:
			puts("list [");

			for (i = 0; node->val.l[i]; ++i)
				_be_dump(node->val.l[i], indent + 1);

			_be_dump_indent(indent);
			puts("]");
			break;

		case BE_DICT:
			puts("dict {");

			for (i = 0; node->val.d[i].val; ++i) {
				_be_dump_indent(indent + 1);
				printf("%s => ", node->val.d[i].key);
				_be_dump(node->val.d[i].val, -(indent + 1));
			}

			_be_dump_indent(indent);
			puts("}");
			break;
	}
}
void be_dump(be_node *node)
{
	_be_dump(node, 0);
}
#endif
