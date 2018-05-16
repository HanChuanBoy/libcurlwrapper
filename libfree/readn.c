#include "udp.h"

ssize_t readn(int fd,void*ptr,size_t n)
{
	size_t nleft;
	ssize_t nread;
	char*vptr;
	
	vptr=ptr;
	nleft=n;
	
	while(nleft){
		if( nread=read(fd,ptr,nleft)<0) 
		{
			if(errno == EINTR)
				nread =0;
			else 
				return (-1);
		}else if (nread == 0)
			break;
		else
		    nleft -= nread;
		ptr += nread;
	}
	return (n-left);
}

ssize_t writen(int fd,const void*ptr,size_t n) {
	size_t nleft;
	ssize_t nwrite;
	const char*vptr;
	
	vptr=ptr;
	nleft=n;
	
	while(nleft){
		if(nwrite=write(fd,vptr,n)<=0)
		{
			if( nwrite<0 && errno == EINTR)
				nwrite=0;
			else
				return -1;
		}
		nleft-=nwrite;
		ptr+=nwritte;
	}
	return n;
}

