#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <string.h>

#include "curl_aml_common.h"

void*c_aml_malloc(unsigned int size) {
	void *ptr=NULL;   //将0转换成指针是可以的;
	ptr = malloc(size);
	return ptr;
}


void *c_aml_realloc(void *ptr, unsigned int size)
{
    return realloc(ptr, size);
}

/*
  释放二级指针,释放二级指针指向内存;
  并在函数内部实现指针的赋空; 
*/
void c_aml_freep(void **arg)
{
    if (*arg) {
        free(*arg);
    }
    *arg = NULL;
}

/*
  指针需要在外部赋空，不能访问指向内存;
*/
void c_aml_free(void *arg)
{
    if (arg) {
        free(arg);
    }
}

/**
  返回值是一个指针;
*/
void *c_aml_mallocz(unsigned int size) {
	void*ptr = malloc(size);
	if(ptr) {
		memset(ptr,0,size);
	}
	return ptr;
}

int c_aml_strstart(const char *str, const char *pfx, const char **ptr)
{
    while (*pfx && *pfx == *str) {
        pfx++;
        str++;
    }
    if (!*pfx && ptr) {
        *ptr = str;
    }
    return !*pfx;
}


/***
  汇编中的x ptr//x指的类型,ptr对应的是地址值;
  int a =10 ; mov dword ptr [a] 0xAh
  char*str="ab" mov dword ptr [str] offset string "ab";
param: 输入字符串,前缀匹配字符串,返回字符串指针;
return:是否对应字符串匹配;
author:hang.xu
*/
int c_aml_stristart(const char* str,const char*pfx,const char**ptr) {
	while(*pfx && toupper((unsigned)*pfx) == toupper((unsigned)*str))
	{
		pfx++;
		str++;
	}
	if ( !*pfx && ptr ) {
		*ptr=str;
	}
	return !*pfx;
}

/**
param:判断s2是不是s1的子字符串;
return:返回字符串指针;
author:hang.xu
*/
char *c_aml_stristr(const char *s1,const char *s2) {
	if(!*s2){
		return s1;
	}
	
	do {
		if(c_aml_stristart(s1,s2,NULL)) {
			return s1;
		}
	} while (*s1++);
	return NULL;
}


/*
param:从右向左进行比较,比较字符串str是不是s的一部分;
return:返回字符串指针;
author:hang.xu
比较常用的地方是ip地址异常的时候;
*/
char *c_aml_strrstr(const char*s,const char *str)
{
	char*p;
	int len=strlen(s);
	for (p=s+len-1;p>=s;p--){
		if ((*p == *str) && (memcmp(p,str,strlen(str)) == 0))
			return p;
	}
	return NULL;
}

/*
param:字符串的拷贝,拷贝对应的源地址,目的地址,大小;
return:返回拷贝的大小;
author:hang.xu
*/
size_t c_aml_strlcpy(char*dst,const char*src,size_t size)
{
	size_t len = 0;
	while( ++len < size && *src) {
		*dst++ = *src++;
	}
	if(len <= size)
	{
		*dst=0;
	}
	return len-1;
}

size_t c_aml_strlcat(char *dst, const char *src, size_t size)
{
    size_t len = strlen(dst);
    if (size <= len + 1) {
        return len + strlen(src);
    }
    return len + c_aml_strlcpy(dst + len, src, size - len);
}


/*
	  typedef char * va_list;     // TC中定义为void*
      #define _INTSIZEOF(n)    ((sizeof(n)+sizeof(int)-1)&~(sizeof(int) - 1) ) //为了满足需要内存对齐的系统
      #define va_start(ap,v)    ( ap = (va_list)&v + _INTSIZEOF(v) )     //ap指向第一个变参的位置，即将第一个变参的地址赋予ap
      #define va_arg(ap,t)       ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )   /*获取变参的具体内容，t为变参的类型，如有多个参数，则通过移动ap的指针来获得变参的地址，从而获得内容
      #define va_end(ap) ( ap = (va_list)0 )   //清空va_list，即结束变参的获*/

/**
param:字符串的拷贝,拷贝对应的源地址,目的地址,大小;
return:返回拷贝的大小;
author:hang.xu
*/
size_t c_aml_strlcatf(char *dst, size_t size, const char *fmt, ...)
{
    int len = strlen(dst);
    va_list vl;

    va_start(vl, fmt);
    len += vsnprintf(dst + len, size > len ? size - len : 0, fmt, vl);
    va_end(vl);
    return len;
}
