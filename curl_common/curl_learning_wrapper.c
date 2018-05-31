#include "curl_learning_wrapper.h"

/**
  掌握双向链表的添加;
  如果是末尾添加;
*/
static int curl_learning_wrapper_add_curl_handle(CurlWContext* con,CurlWHandle*h)
{
     int ret = -1;
     if (!con || !h) {
        CLOGE("curl_wrapper_add_curl_handle invaild handle");
        return ret;
     }
     CurlWHandle*tmp_h;
     CurlWHandle*prev_h;
     tmp_h= con->handle;
     prev_h = NULL;
     while (tmp_h) {
        prev_h = tmp_h;
        tmp_h = tmp_h->next;
     }
     prev_h->next = h;
     h->prev = prev_h;  
     h->next = NULL;
     con->curl_h_num++;
     ret = 0;
     return ret;
}

/**
   
*/
static int curl_learning_wrapper_del_curl_handle(CurlWContext* con,CurlWHandle*h)
{
   int ret=-1;
   if(!con || !h) {
     CLOGE("curl_wrapper_del_curl_handle invalid handle\n");
     return ret;
   }
   CurlWHandle*tmp_h;
   for (tmp_h = con->curl_handle; tmp_h; tmp_h = tmp_h->next) {
        if (tmp_h == h) {
            break;
        }
    }
    if (!tmp_h) {
        CLOGE("could not find curl handle\n");
        return ret;
    }
    if (con->curl_handle == h) {
        con->curl_handle = h->next;
        if (con->curl_handle) {
            con->curl_handle->prev = NULL;
        }
    } else {
        tmp_h = h->prev;
        tmp_h->next = h->next;
        if (h->next) {
            h->next->prev = tmp_h;
        }
    }
    con->curl_h_num--;
    if (h->cfifo) {
        curl_fifo_free(h->cfifo);
    }
    pthread_mutex_destroy(&h->fifo_mutex);
    pthread_mutex_destroy(&h->info_mutex);
    pthread_cond_destroy(&h->pthread_cond);
    pthread_cond_destroy(&h->info_cond);
    if (h->curl) {
        curl_easy_cleanup(h->curl);
        h->curl = NULL;
    }
    if (h->relocation) {
        c_free(h->relocation);
        h->relocation = NULL;
    }
    if (h->get_headers) {
        c_free(h->get_headers);
        h->get_headers = NULL;
    }
    if (h->post_headers) {
        c_free(h->post_headers);
        h->post_headers = NULL;
    }
    c_free(h);
    h = NULL;
    ret = 0;
    return ret;
}

/**
   old version;
*/
static int curl_learning_wrapper_del_all_curl_handle(CurlWContext*con) {
   int ret = -1;
   if (!con) {
      CLOGE("curl_wrapper_del_all_curl_handle invalid handle\n"); 
      return ret;
   }
   CurlWHandle*temp_p=NULL;
   CurlWHandle*temp_h=NULL;
   tmp_p=con->curl_handle;
   while (tmp_p) {
     tmp_p=temp_h->next;
     con->curl_h_num--;
     if (tmp_p->cfifo) {
         curl_fifo_free(tmp_p->cfifo);
         pthread_mutex_destroy(&tmp_p->fifo_mutex);
         pthread_mutex_destroy(&tmp_p->info_mutex);
         pthread_cond_destroy(&tmp_p->pthread_cond);
         pthread_cond_destroy(&tmp_p->info_cond);
     }
     if (tmp_p->relocation) {
            c_free(tmp_p->relocation);
            tmp_p->relocation = NULL;
        }
        if (tmp_p->get_headers) {
            c_free(tmp_p->get_headers);
            tmp_p->get_headers = NULL;
        }
        if (tmp_p->post_headers) {
            c_free(tmp_p->post_headers);
            tmp_p->post_headers = NULL;
        }
        c_free(tmp_p);
        tmp_p = NULL;
        tmp_p = tmp_t;
   }
   con->curl_handle = NULL;
   ret = 0;
   return ret;
}

