#include "udp.h"

static int read_cnt;
static char*read_ptr;
static char read_buf[MAX_LINE];

/*
   ��ȡ�׽��ֵ��ı�����;�ӻ������ж�ȡ����;
   
*/
static ssize_t myread(int fd,char*ptr)
{
	if(read_cnt <= 0){
again:
		if((read_cnt = read(fd,read_buf,sizeof(read_buf)))<0) {
			if(errno == EINTR) {
				goto again;
			} else if (read_cnt==0)
				return (0);
			read_ptr = read_buf;
		}
		read_cnt--;
		*ptr=*read_ptr++;
		return 1;
}

/*
   ����Ҫ��ȡ��������,���ַ��Ŀ�������;
*/
ssize_t readline(int fd,void*vptr,size_t maxlen){
	ssize_t n,rc;
	char c,*ptr;
	
	ptr=vptr;
	for(n=1; n<maxlen; n++) {
		if((rc = myread(fd,&c)==1)){
			*ptr++=c;
			if(c== '\n')
				break;
			else if(rc==0) {
				*ptr=0;
				return n-1;
			}
			else
				return -1;
		}
	}
	*ptr=0;
	return n;
}

/*
   Ŀ����Ҫ�Ǹ��¶�дָ��;
*/
ssize_t readlinbuf(void**vptrvptr){
	if(read_cnt) *vptrptr=read_ptr;
	return read_cnt;
}

