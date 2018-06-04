#ifndef CURL_WRAPPER_H
#define CURL_WRAPPER_H
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>

#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include "curl/curl.h"
#include "curl_aml_common.h"


#include "curl_fifo.h"
#include "curl_enum.h"

typedef void (*infonotifycallback)(void * info, int * ext);
typedef int (*interruptcallback)(void);
typedef int (*interruptcallbackwithpid)(void *);

typedef struct _CurlWHandle {
    char uri[MAX_CURL_URI_SIZE];
    char curl_setopt_error[CURL_ERROR_SIZE];
    char * relocation;
    char * get_headers;
    char * post_headers;
    int quited;
    int open_quited;
    int redirect_quited;
    int c_max_timeout;
    int c_max_connecttimeout;
    int c_buffersize;
    int http_code;
    int perform_error_code;
    int seekable;
    CURL *curl;
    Curlfifo *cfifo;
    int64_t chunk_size;
    double dl_speed;
    void (*infonotify)(void * info, int * ext);
    int (*interrupt)(void);
    int (*interruptwithpid)(void *);
    void * parent_thread_id;
    pthread_mutex_t fifo_mutex;
    pthread_mutex_t info_mutex;
    pthread_cond_t pthread_cond;
    pthread_cond_t info_cond;
    struct _CurlWHandle * prev;
    struct _CurlWHandle * next;
} CurlWHandle;

typedef struct _CurlWContext {
    int quited;
    int open_fail;
    int curl_h_num;
    int chunked;
    int connected;
    int is_use_block_request;
    int ignore_interrupt;
    int clear_redirect_url;
    int (*interrupt)(void);
    int (*interruptwithpid)(void *);
    void * parent_thread_id;
    CURLM *multi_curl;
    CurlWHandle * curl_handle;
} CurlWContext;

typedef struct _Curl_Data {
    int64_t size;
    CurlWHandle * handle;
    CurlWContext * ctx;
    int rangesize;
    char rangebuf[128];
} Curl_Data;

CurlWContext * curl_wrapper_init(int flags);
CurlWHandle * curl_wrapper_open(CurlWContext * handle, const char * uri, const char * headers, Curl_Data * buf, curl_prot_type flags);
int curl_learning_wrapper_http_keepalive_open(CurlWContext * con, CurlWHandle * h, const char * uri);
int curl_learning_wrapper_perform(CurlWContext * handle);
int curl_learning_wrapper_read(CurlWContext * handle, uint8_t * buf, int size);
int curl_learning_wrapper_write(CurlWContext * handle, const uint8_t * buf, int size);
int curl_learning_wrapper_seek(CurlWContext * con, CurlWHandle * handle, int64_t off, Curl_Data *buf, curl_prot_type flags);
int curl_learning_wrapper_close(CurlWContext * handle);
int curl_learning_wrapper_set_para(CurlWHandle * handle, void * buf, curl_para para, int iarg, const char * carg);
int curl_learning_wrapper_clean_after_perform(CurlWContext * handle);
int curl_learning_wrapper_set_to_quit(CurlWContext * con, CurlWHandle * h);
int curl_learning_wrapper_get_info_easy(CurlWHandle * handle, curl_info cmd, uint32_t flag, int64_t * iinfo, char * cinfo);
int curl_learning_wrapper_get_info(CurlWHandle * handle, curl_info cmd, uint32_t flag, void * info);
int curl_learning_wrapper_register_notify(CurlWHandle * handle, infonotifycallback pfunc);

#endif

