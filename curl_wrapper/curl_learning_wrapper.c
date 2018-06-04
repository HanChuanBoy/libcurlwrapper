#include "curl_learning_wrapper.h"
#include "curl_log.h"
#include "curl/curl.h"
#define CURL_FIFO_BUFFER_SIZE 2*1024*1024

#define CURL_LOCATION_LEN 1024
#define CURL_WASHU_DOMAIN_NAME "http://gslbserv.itv.cmvideo.cn"
static int s_LocationNum = 0;
static char s_UrlKey[CURL_LOCATION_LEN] = {0};
static char s_UrlValue[CURL_LOCATION_LEN] = {0};

int am_getconfig_int_def(const char * path, int def)
{
   return 0;
}

/**
  CurlWContext
  CurlWHandle;
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
     tmp_h= con->curl_handle;
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
   used to parse headdata;
*/
static void response_process(char * line, Curl_Data * buf)
{
    if (line[0] == '\0') {
        return;
    }
    CLOGI("[response]: %s", line);
    char * ptr = line;
    if (!strncasecmp(line, "HTTP", 4)) {
        while (!isspace(*ptr) && *ptr != '\0') {
            ptr++;
        }
        buf->handle->http_code = strtol(ptr, NULL, 10);
        return;
    }
    if (200 == buf->handle->http_code
        || 206 == buf->handle->http_code) {
        if (!buf->ctx->connected) {
            buf->ctx->connected = 1;
        }
        if (!strncasecmp(line, "Content-Length", 14)&& buf->handle->chunk_size == -1) {
            while (*ptr != '\0' && *ptr != ':') {
                ptr++;
            }
            ptr++;
            while (isspace(*ptr)) {
                ptr++;
            }
            buf->handle->chunk_size = atoll(ptr);
        }
        if (!strncasecmp(line, "Content-Range", 13)) {
            const char * slash = NULL;
            if ((slash = strchr(ptr, '/')) && strlen(slash) > 0) {
                buf->handle->chunk_size = atoll(slash + 1);
            }
        }
        if (!strncasecmp(line, "Transfer-Encoding", 17) && strstr(line, "chunked")) {
            buf->ctx->chunked = 1;
        }
        if (!strncasecmp(line, "\n", 1)
            || !strncasecmp(line, "\r", 1)) {
            buf->handle->open_quited = 1;
        }
        return;
    }
    if (302 == buf->handle->http_code
        || 301 == buf->handle->http_code
        || 303 == buf->handle->http_code
        || 307 == buf->handle->http_code) {
        if (!strncasecmp(line, "Location", 8)) {
            char * tmp = NULL;
            while (*ptr != '\0' && *ptr != ':') {
                ptr++;
            }
            ptr++;
            while (isspace(*ptr)) {
                ptr++;
            }
            
            tmp = strchr(ptr, '\?');
            if (s_LocationNum == 2 && s_UrlKey[0] != '\0' && s_UrlValue[0] != '\0'){
                CLOGI("------------------Location need redirect !!");
                char temp_uri[CURL_LOCATION_LEN] = {0};
                if (tmp) {
                    memset(temp_uri, 0, sizeof(temp_uri));
                    strcpy(temp_uri, s_UrlValue);
                    strcpy(&temp_uri[strlen(s_UrlValue)], tmp);
                } else {
                    strcpy(temp_uri, ptr);
                    CLOGI("------------------redirect error, cannot find ?---------");
                }

                buf->handle->relocation = (char *)c_aml_mallocz(strlen(temp_uri) + 1);
                if (!buf->handle->relocation) {
                    return;
                }
                strcpy(buf->handle->relocation, temp_uri);
                if (am_getconfig_int_def("libplayer.curl.error_redirect", 0)) {
                    buf->handle->relocation[8] = '1';
                    buf->handle->relocation[9] = '2';
                }
                buf->handle->redirect_quited = 1;
            } else if (s_LocationNum == 1 && s_UrlKey[0] != '\0') {
                CLOGI("------------------Location save");
                if (tmp && (tmp-ptr) < CURL_LOCATION_LEN) {
                    strncpy(s_UrlValue, ptr, (tmp-ptr));
                    s_UrlValue[tmp-ptr] = '\0';
                    CLOGI("------------------[value=%s]", s_UrlValue);
                }

                buf->handle->relocation = (char *)c_aml_mallocz(strlen(ptr) + 1);
                if (!buf->handle->relocation) {
                    return;
                }
                strcpy(buf->handle->relocation, ptr);
            } else {
                buf->handle->relocation = (char *)c_aml_mallocz(strlen(ptr) + 1);
                if (!buf->handle->relocation) {
                    return;
                }
                strcpy(buf->handle->relocation, ptr);
            }

            tmp = buf->handle->relocation;
            while (*tmp != '\0') {
                if (*tmp == '\r' || *tmp == '\n') {
                    *tmp = '\0';
                    break;
                }
                tmp++;
            }
            strcpy(buf->handle->uri, buf->handle->relocation);
        }
        return;
    }
    if (c_aml_stristr(line, "Octoshape-Ondemand")) {
        buf->handle->seekable = 1;
        return;
    }
    if (buf->handle->http_code >= 400 && buf->handle->http_code < 600) {
        buf->handle->open_quited = 1;
    }
}


/*
   this function gets called by libcurl as soon as it has recived head data;
   once called for each header;
   the size of ptr is size multipled nmenb;
   CURLOPT_HEADERDATA  will return a amount;
---->CURLOPT_HEADERFUNCTION  amount will pass to function;
   the size of data pass in CURLOPT_HEADERDATA  function callback return;
   equal to the size*nmemb;
*/
static size_t write_response(void *ptr, size_t size, size_t nmemb, void *data)
{
    size_t realsize = size * nmemb;
    if (realsize <= 0) {
        return 0;
    }
    Curl_Data * mem = (Curl_Data *)data;
    if (mem->handle->quited) {
        CLOGI("write_response quited\n");
        return -1;
    }
    char * tmp_ch = c_aml_malloc(realsize + 1);
    if (!tmp_ch) {
        return -1;
    }
    c_aml_strlcpy(tmp_ch, ptr, realsize);
    tmp_ch[realsize] = '\0';
    response_process(tmp_ch, mem);
    c_aml_free(tmp_ch);
    return realsize;
}


/**
   store some domain through uri;
*/
static int cache_domain_name(CurlWContext *h, const char *uri)
{
    char *temp1 = strstr(uri, "duration");
    char *temp2 = NULL;
    if (!am_getconfig_int_def("libplayer.curl.force_redirect", 0))
    {
        CLOGI("cache_domain_name no force_redirect\n");
        return 0;
    }

    char duration_str[64] = {0};
    if (!temp1)
        return 0;
    temp2 = strchr(temp1, '\&');
    if (!temp2 || (temp2-temp1-9) >= sizeof(duration_str))
        return 0;

    memcpy(duration_str, temp1+9, (temp2-temp1-9));
    CLOGI("cache_domain_name  duration=%s\n", duration_str);
    //domain + duration
    if (0 == strncmp(uri, CURL_WASHU_DOMAIN_NAME, strlen(CURL_WASHU_DOMAIN_NAME))
        && 0 != strcmp(duration_str, "0") && 0 != strcmp(duration_str, "-1")) {
        if (s_UrlKey[0] != '\0'
            && 0 == strncmp(uri, s_UrlKey, strlen(s_UrlKey))) {
            if (h->clear_redirect_url) {
                s_UrlValue[0] = '\0';
                CLOGI("cache_domain_name  clear_redirect_url\n");
                return 1;
            }
            return 2;
        } else {
            //new program
            char *temp = strchr(uri, '\?');
            if (temp && (temp-uri) < CURL_LOCATION_LEN) {
                strncpy(s_UrlKey, uri, (temp-uri));
                s_UrlKey[temp-uri] = '\0';
                s_UrlValue[0] = '\0';
                CLOGI("cache_domain_name  add new url.[key=%s]\n", s_UrlKey);
                return 1;
            }
        }
    }
    return 0;
}


static int curl_wrapper_setopt_error(CurlWHandle *h, CURLcode code)
{
    int ret = -1;
    if (code != CURLE_OK) {
        if (!h) {
            CLOGE("Invalid CURLWHandle\n");
            return ret;
        }
        CLOGE("curl_easy_setopt failed : [%s]\n", h->curl_setopt_error);
        return ret;
    }
    ret = 0;
    return ret;
}


/**
   there are two kinds of method to get debug info;
   one is stderr; one is debug info;
*/
static int OnDebug(CURL *tmpcurl, curl_infotype itype, char * pData, size_t size, void *data)  
{  
    Curl_Data * mem = (Curl_Data *)data;
    if(itype == CURLINFO_TEXT)  
    {  
        CLOGI("[TEXT]%s\n", pData);  
    }  
    else if(itype == CURLINFO_HEADER_IN)  
    {  
        //CLOGI("[HEADER_IN]%s\n", pData);  
    }  
    else if(itype == CURLINFO_HEADER_OUT)  
    {
        if(size>0&&pData!=NULL){
            char * tmp_ch =(char*)c_aml_malloc(size + 2);
            if (!tmp_ch) {
                return -1;
            }
            c_aml_strlcpy(tmp_ch, pData, size);
            tmp_ch[size] = '\0';
            CLOGI("[HEADER_OUT]%s\n", tmp_ch);  
            c_aml_free(tmp_ch);
            tmp_ch=NULL;
        }
        else{
            CLOGI("[HEADER_OUT] is NULL\n");
        }

    }  
    else if(itype == CURLINFO_DATA_IN)  
    {  
        //CLOGI("[DATA_IN]%s\n", pData);  
    }  
    else if(itype == CURLINFO_DATA_OUT)  
    {  
        //CLOGI("[DATA_OUT]%s\n", pData);  
    }  
    return 0;  
}

/**
   add a  handle to get curl http;
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
        c_aml_free(h->relocation);
        h->relocation = NULL;
    }
    if (h->get_headers) {
        c_aml_free(h->get_headers);
        h->get_headers = NULL;
    }
    if (h->post_headers) {
        c_aml_free(h->post_headers);
        h->post_headers = NULL;
    }
    c_aml_free(h);
    h = NULL;
    ret = 0;
    return ret;
}

/**
   write CURL_MAX_WRITE_SIZE is default by 16K;
   define in curl.h head file;
   you can modify the vaule by set other if CURLOPT_HEADER is set;
   the value will be 100K when CURLOPT_HEADERFUNCTION is not set;
   
   the function can be stock when curl_fifo_space don’t changes;
*/
static size_t curl_learning_dl_chunkdata_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
    size_t realsize = size * nmemb;
    if (realsize <= 0) {
        return 0;
    }
    Curl_Data * mem = (Curl_Data *)data;
    mem->handle->open_quited = 1;
    if (mem->handle->quited || mem->ctx->quited) {
        CLOGI("curl_dl_chunkdata_callback quited\n");
        return -1;
    }

    pthread_mutex_lock(&mem->handle->fifo_mutex);
    int left = curl_fifo_space(mem->handle->cfifo);
    while (left <= 0 || left < realsize) {
        if (mem->handle->quited) {
            CLOGI("curl_dl_chunkdata_callback quited loop\n");
            pthread_mutex_unlock(&mem->handle->fifo_mutex);
            return -1;
        }
        if (mem->handle->interrupt) {
            if ((*(mem->handle->interrupt))() && !mem->ctx->ignore_interrupt) {
                CLOGI("curl_dl_chunkdata_callback interrupted\n");
                pthread_mutex_unlock(&mem->handle->fifo_mutex);
                return -1;
            }
        }
        if (mem->handle->interruptwithpid) {
            if ((*(mem->handle->interruptwithpid))(mem->handle->parent_thread_id) && !mem->ctx->ignore_interrupt) {
                CLOGI("curl_dl_chunkdata_callback interruptedwithpid\n");
                pthread_mutex_unlock(&mem->handle->fifo_mutex);
                return -1;
            }
        }

        struct timeval now;
        struct timespec timeout;
        gettimeofday(&now, NULL);
        timeout.tv_sec = now.tv_sec + (50 * 1000 + now.tv_usec) / 1000000;
        //timeout.tv_nsec = now.tv_usec * 1000;
        timeout.tv_nsec = (50 * 1000 + now.tv_usec) * 1000;
        pthread_cond_timedwait(&mem->handle->pthread_cond, &mem->handle->fifo_mutex, &timeout); // 100ms
        left = curl_fifo_space(mem->handle->cfifo);
    }
    curl_fifo_generic_write(mem->handle->cfifo, ptr, realsize, NULL);
    pthread_mutex_unlock(&mem->handle->fifo_mutex);
    mem->size += realsize;
    return realsize;
}


/**
   delete all handle in curlwcontext;
   traverse the con->handle;
   each handle has a cfifo;
   each handle has a pthread_condtion and a info_condtion;
   each handle has a pthread_mutex;
*/
static int curl_learning_wrapper_del_all_curl_handle(CurlWContext*con) {
   int ret = -1;
   if (!con) {
      CLOGE("curl_wrapper_del_all_curl_handle invalid handle\n"); 
      return ret;
   }
   CurlWHandle*tmp_p=NULL;
   CurlWHandle*tmp_t=NULL;
   tmp_p=con->curl_handle;
   while (tmp_p) {
     tmp_t=tmp_p->next;
     con->curl_h_num--;
     if (tmp_p->cfifo) {
         curl_fifo_free(tmp_p->cfifo);
         pthread_mutex_destroy(&tmp_p->fifo_mutex);
         pthread_mutex_destroy(&tmp_p->info_mutex);
         pthread_cond_destroy(&tmp_p->pthread_cond);
         pthread_cond_destroy(&tmp_p->info_cond);
     }
     if (tmp_p->relocation) {
            c_aml_free(tmp_p->relocation);
            tmp_p->relocation = NULL;
        }
        if (tmp_p->get_headers) {
            c_aml_free(tmp_p->get_headers);
            tmp_p->get_headers = NULL;
        }
        if (tmp_p->post_headers) {
            c_aml_free(tmp_p->post_headers);
            tmp_p->post_headers = NULL;
        }
        c_aml_free(tmp_p);
        tmp_p = NULL;
        tmp_p = tmp_t;
   }
   con->curl_handle = NULL;
   ret = 0;
   return ret;
}

/**
    curl_learning_wrapper_init
    to new a context and get a multicurl  pointer to be a member;
    is_use_block_request;
	
*/
CurlWContext* curl_learning_wrapper_init(int flags) {
     CLOGI("curl_wrapper_init enter\n");
     CurlWContext*  context = NULL;
     context = (CurlWContext *)c_aml_malloc(sizeof(CurlWContext));
     if (!context) {
         CLOGE("Failed to allocate memory for CURLWContext handle\n");
         return NULL;
     }
     context->multi_curl = curl_multi_init();
     if (!context->multi_curl) {
        CLOGE("CURLWContext multi_curl init failed\n");
        return NULL;
     }
     context->parent_thread_id = NULL;
     context->curl_h_num = 0;
     context->curl_handle = NULL;
     context->clear_redirect_url = flags;
     s_LocationNum = 0;
     context->is_use_block_request=am_getconfig_int_def("media.libplayer.useblock",0);
     return context;
}

/**
     http_basic;
CURLOPT_VERBOSE; to show some verbose in stderr;
CURLOPT_DEBUGDATA; set a useptr used to be container;

CURLOPT_DEBUGFUNCTION; pass a pointer to your callback function
which should match the prototype shown above;

CURLOPT_HTTP_VERSION;
which version we used to be;
version 1;  restrict the connect not be persistent;
when explore start we must install a tcp shake hands;
if you want to build a persistent connect you can request a Keep-alive header;

vesrion 1.1; support persistent Connection and pipelining
             support chunked transfer way;
version 2.0; support high Concurrency technology;
             support HPACK Compression algorithm;

CURLOPT_FOLLOWLOCATION;
   suppor hups in different host;

CURLOPT_CONNECTTIMEOUT;
   what default 300s;

CURLOPT_TIMEOUT;
   limit a hard timeout in process to transfering;

CURLOPT_BUFFERSIZE;
  max vaule is 512k;min value is 1k;default is 16k;
  is used under circumstance when chunk callback has occurr many times;
  
CURLOPT_FORBID_REUSE;
  curl doen't active close the perform when you are in multi handle;
  
CURLOPT_FRESH_CONNECT;
  not to use a establish connection;
  netstat -n | awk '/^tcp/ {++S[$NF]} END {for(a in S) print a, S[a]}'   

CURLOPT_NOSIGNAL;
  must be add 1L for multi curl handle;
  
CURLOPT_URL;
CURLOPT_WRITEDATA;
CURLOPT_WRITEFUNCTION;
  to set write data and writedata function; 

the point we have to notice is that we must new a connect to a new curl_easy_perform();
when CURLOPT_FORBID_REUSE is set 1;

*/
static int curl_learning_wrapper_easy_setopt_http_basic(CurlWHandle *h, Curl_Data *buf)
{
    CLOGI("curl_wrapper_easy_setopt_http_basic enter\n");
    int ret = -1;
    if (!h) {
        CLOGE("CurlWHandle invalid\n");
        return ret;
    }
    CURLcode easy_code;
    easy_code = curl_easy_setopt(h->curl, CURLOPT_ERRORBUFFER, h->curl_setopt_error);
    if (easy_code != CURLE_OK) {
        CLOGE("CurlWHandle easy setopt errorbuffer failed\n");
        return ret;
    }
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_VERBOSE, 1L))) != 0) {
        goto CRET;
    }
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_DEBUGDATA, (void *)buf))) != 0) {
        goto CRET;
    }
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_DEBUGFUNCTION, OnDebug))) != 0) {
        goto CRET;
    }
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1))) != 0) {
        goto CRET;
    }
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_FOLLOWLOCATION, s_LocationNum==2?0L:1L))) != 0) {
        goto CRET;
    }
    CLOGI("curl_wrapper_easy_setopt_http_basic CURLOPT_FOLLOWLOCATION %ld, s_LocationNum=%d\n", s_LocationNum==2?0L:1L, s_LocationNum);
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_CONNECTTIMEOUT, h->c_max_connecttimeout))) != 0) {
        goto CRET;
    }
    /*
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_TIMEOUT, h->c_max_timeout))) != 0) {
        goto CRET;
    }
    */
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_BUFFERSIZE, h->c_buffersize))) != 0) {
        goto CRET;
    }
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_FORBID_REUSE, 1L))) != 0) {
        goto CRET;
    }
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_FRESH_CONNECT, 1L))) != 0) {
        goto CRET;
    }
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_NOSIGNAL, 1L))) != 0) {
        goto CRET;
    }
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_URL, h->uri))) != 0) {
        goto CRET;
    }
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_WRITEDATA, (void *)buf))) != 0) {
        goto CRET;
    }
    if ((ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_WRITEFUNCTION, curl_learning_dl_chunkdata_callback))) != 0) {
        goto CRET;
    }
CRET:
    return ret;
}


/**
   for a handle ;
   curl ---> curl_easy_init(); to be a member of handle;
   what curl_port_type differ is C_PROT_HTTP or C_PROT_HTTPS
   CURLOPT_RANGE used by block request;
   CURLOPT_RESUME_FROM_LARGE;we don't use block request;
*/
static int curl_learning_wrapper_open_cnx(CurlWContext *con, CurlWHandle *h, Curl_Data *buf, curl_prot_type flags, int64_t off)
{
    CLOGI("curl_wrapper_open_cnx enter\n");
    int ret = -1;
    if (!con || !h) {
        CLOGE("Invalid CURLWHandle\n");
        return ret;
    }
    if (!h->curl) {
        h->curl = curl_easy_init();
        if (!h->curl) {
            CLOGE("CURLWHandle easy init failed\n");
            return ret;
        }
    }
    if (flags == C_PROT_HTTP || flags == C_PROT_HTTPS) {
        ret = curl_learning_wrapper_easy_setopt_http_basic(h, buf);
        if (ret != 0) {
            CLOGE("curl_wrapper_easy_setopt_http_basic failed\n");
            return ret;
        }
    }
    if (flags == C_PROT_HTTPS) {
        //curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_SSL_VERIFYPEER, 0L));
        //curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_SSL_VERIFYHOST, 0L));
        curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_CAINFO, "/etc/curl/cacerts/ca-certificates.crt"));
    }
    memset(buf->rangebuf,0,sizeof(buf->rangebuf));
    if(con->is_use_block_request)
    {
        CLOGI("%s %d use block request\n",__FUNCTION__,__LINE__);
        if(((off+buf->rangesize)>h->chunk_size)&&(h->chunk_size>0)){
            sprintf(buf->rangebuf,"%lld-%lld",off,h->chunk_size-1);
        }
        else{
            sprintf(buf->rangebuf,"%lld-%lld",off,off+buf->rangesize);
        }

        curl_easy_setopt(h->curl, CURLOPT_RANGE, buf->rangebuf);
    }
    else{
        CLOGI("%s %d do not use block request\n",__FUNCTION__,__LINE__);
        if (h->chunk_size > 0) {
           // not support this when transfer in block mode.
           ret = curl_easy_setopt(h->curl, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)off);
        }
    }
    //curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_ACCEPT_ENCODING, "gzip"));
    con->quited = 0;
    con->chunked = 0;
    con->connected = 0;
    con->open_fail = 0;
    con->ignore_interrupt = 0;
    h->quited = 0;
    h->open_quited = 0;
    h->redirect_quited = 0;
    h->seekable = 0;
    h->perform_error_code = 0;
    h->dl_speed = 0.0f;
    buf->handle = h;
    buf->ctx = con;
    buf->size = off ? off : 0;
    ret = 0;
    return ret;
}

/**
  must have a param uri,header;
  open a url and new a handle to be a member of context;
  for a handle what we need set;
  cfifo;
  at last we callback curl_leanring_wrapper_open_cnx to get some param of handle;

*/
CurlWHandle * curl_learning_wrapper_open(CurlWContext *h, const char * uri, const char * headers, Curl_Data *buf, curl_prot_type flags)
{
    CLOGI("curl_wrapper_open enter\n");
    if (!h) {
        CLOGE("Invalid CurlWContext handle\n");
        return NULL;
    }
    if (!uri || strlen(uri) < 1 || strlen(uri) > MAX_CURL_URI_SIZE) {
        CLOGE("Invalid CurlWContext uri path\n");
        return NULL;
    }
    CurlWHandle * curl_h= (CurlWHandle *)c_aml_malloc(sizeof(CurlWHandle));
    if (!curl_h) {
        CLOGE("Failed to allocate memory for CurlWHandle\n");
        return NULL;
    }
    curl_h->infonotify = NULL;
    curl_h->interrupt = NULL;
    if (curl_learning_wrapper_add_curl_handle(h, curl_h) == -1) {
        return NULL;
    }
    memset(curl_h->uri, 0, sizeof(curl_h->uri));
    memset(curl_h->curl_setopt_error, 0, sizeof(curl_h->curl_setopt_error));
    c_aml_strlcpy(curl_h->uri, uri, sizeof(curl_h->uri));
    curl_h->curl = NULL;
    curl_h->cfifo = curl_fifo_alloc(CURL_FIFO_BUFFER_SIZE);
    if (!curl_h->cfifo) {
        CLOGE("Failed to allocate memory for curl fifo\n");
        return NULL;
    }
    pthread_mutex_init(&curl_h->fifo_mutex, NULL);
    pthread_mutex_init(&curl_h->info_mutex, NULL);
    pthread_cond_init(&curl_h->pthread_cond, NULL);
    pthread_cond_init(&curl_h->info_cond, NULL);
    curl_h->relocation = NULL;
    curl_h->get_headers = NULL;
    curl_h->post_headers = NULL;
    curl_h->chunk_size = -1;
    curl_h->c_max_timeout = 0;
    curl_h->c_max_connecttimeout = 15;
    curl_h->c_buffersize = 16 * 1024;
    s_LocationNum = cache_domain_name(h, uri);
    if (curl_learning_wrapper_open_cnx(h, curl_h, buf, flags, 0) == -1) {
        return NULL;
    }
    return curl_h;
}


/**
*/
int curl_learning_wrapper_set_para(CurlWHandle *h, void *buf, curl_para para, int iarg, const char * carg)
{
    CLOGI("curl_wrapper_set_para enter\n");
    int ret = -1, ret1 = -1, ret2 = -1;
    char * tmp_h = NULL;
    if (!h) {
        CLOGE("CurlWHandle invalid\n");
        return ret;
    }
    if (!h->curl) {
        CLOGE("CurlWHandle curl handle not inited\n");
        return ret;
    }
    switch (para) {
    case C_MAX_REDIRECTS:
        ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_MAXREDIRS, iarg));
        break;
    case C_MAX_TIMEOUT:
        h->c_max_timeout = iarg;
        ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_TIMEOUT, iarg));
        break;
    case C_MAX_CONNECTTIMEOUT:
        h->c_max_connecttimeout = iarg;
        ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_CONNECTTIMEOUT, iarg));
        break;
    case C_BUFFERSIZE:
        h->c_buffersize = iarg;
        ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_BUFFERSIZE, iarg));
        break;
    case C_USER_AGENT:
        if (carg) {
            ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_USERAGENT, carg));
        }
        break;
    case C_COOKIES:
        if (carg) {
            ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_COOKIE, carg));
        }
        break;
    case C_HTTPHEADER:
        ret = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_HTTPHEADER, (struct curl_slist *)buf));
        break;
    case C_HEADERS:
        ret1 = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_HEADERFUNCTION, write_response));
        ret2 = curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_WRITEHEADER, buf));
        ret = (ret1 || ret2) ? -1 : 0;
        break;
    default:
        return ret;
    }
    return ret;
}


/**
  to keep a handle reopen or a uri reopen;
*/
int curl_learning_wrapper_http_keepalive_open(CurlWContext*con,CurlWHandle*h,const char*uri) {
    CLOGI("curl_wrapper_keepalive_open enter\n");
    int ret = -1;
    if(!con || !h) {
       CLOGE("Invalid CURLWHandle\n");
       return ret;
    }
    con->quited=0;
    con->chunked=0;
    con->connected=0;
    con->open_fail=0;
    con->ignore_interrupt=0;
    h->quited=0;
    h->open_quited=0;
    h->seekable=0;
    h->perform_error_code=0;
    h->dl_speed=0.0f;
    ret=0;
    if (uri) {
        memset(h->uri,0,sizeof(h->uri));
        memset(h->curl_setopt_error,0,sizeof(h->curl_setopt_error));
        c_aml_strlcpy(h->uri,uri,sizeof(h->uri));
        ret=curl_wrapper_setopt_error(h, curl_easy_setopt(h->curl, CURLOPT_URL, h->uri));
    }
    return ret;
}

int curl_learning_wrapper_read(CurlWContext * h, uint8_t * buf, int size)
{
    CLOGI("curl_wrapper_read enter\n");
    return 0;
}

/**
   to perform curl_multi_add_handle;
   set max timeout 100s;
   we can curl_multi_fdset to set fdwrite and fdread;
   use select way to get a vaild socket;
   we can use epoll;
*/
int curl_learning_wrapper_perform(CurlWContext *con)
{
    CLOGI("curl_wrapper_perform enter\n");
    if (!con) {
        CLOGE("CurlWContext invalid\n");
        return C_ERROR_UNKNOW;
    }
    if (!con->multi_curl) {
        CLOGE("Invalid multi curl handle\n");
        return C_ERROR_UNKNOW;
    }
    CurlWHandle * tmp_h = NULL;
    for (tmp_h = con->curl_handle; tmp_h; tmp_h = tmp_h->next) {
        if (CURLM_OK != curl_multi_add_handle(con->multi_curl, tmp_h->curl)) {
            return C_ERROR_UNKNOW;
        }
    }

    if (con->interrupt) {
        if ((*(con->interrupt))() && !con->ignore_interrupt) {
            CLOGI("[%s:%d] interrupted when multi perform\n", __FUNCTION__, __LINE__);
            return C_ERROR_OK; // prevent read fail.
        }
    }
    if (con->interruptwithpid) {
        if ((*(con->interruptwithpid))(con->parent_thread_id) && !con->ignore_interrupt) {
            CLOGI("[%s:%d] interruptedwithpid when multi perform\n", __FUNCTION__, __LINE__);
            return C_ERROR_OK; // prevent read fail.
        }
    }

    long multi_timeout = 100;
    int running_handle_cnt = 0;
    int select_zero_cnt = 0;
    int select_breakout_flag = 0;
    curl_multi_timeout(con->multi_curl, &multi_timeout);
    curl_multi_perform(con->multi_curl, &running_handle_cnt);
    
    int select_max_retry_time = am_getconfig_int_def("media.curl.max_retry_time", 50);

    while (running_handle_cnt) {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;
        if (select_zero_cnt > 0 && con->connected) {
            tv.tv_usec = 50 * 1000;
        }
        int max_fd;

        fd_set fd_read;
        fd_set fd_write;
        fd_set fd_except;

        if (con->quited) {
            CLOGI("curl_wrapper_perform quited when multi perform\n");
            break;
        }
        if (con->interrupt) {
            if ((*(con->interrupt))() && !con->ignore_interrupt) {
                CLOGI("[%s:%d] interrupted when multi perform\n", __FUNCTION__, __LINE__);
                break;
            }
        }
        if (con->interruptwithpid) {
            if ((*(con->interruptwithpid))(con->parent_thread_id) && !con->ignore_interrupt) {
                CLOGI("[%s:%d] interruptedwithpid when multi perform\n", __FUNCTION__, __LINE__);
                break;
            }
        }

        FD_ZERO(&fd_read);
        FD_ZERO(&fd_write);
        FD_ZERO(&fd_except);

        curl_multi_fdset(con->multi_curl, &fd_read, &fd_write, &fd_except, &max_fd);
        int rc = select(max_fd + 1, &fd_read, &fd_write, &fd_except, &tv);
        switch (rc) {
        case -1:
            CLOGE("curl_wrapper_perform select error\n");
            break;
        case 0:
            select_zero_cnt++;
            CLOGE("curl_wrapper_perform select retrun 0, cnt=%d!\n", select_zero_cnt);
            break;
        default:
            select_zero_cnt = 0;
            while (CURLM_CALL_MULTI_PERFORM == curl_multi_perform(con->multi_curl, &running_handle_cnt)) {
                CLOGI("curl_wrapper_perform runing_handle_count : %d \n", running_handle_cnt);
            }
            break;
        }
        // need to do read seek in upper layer if chunked.
        if (con->connected/* && !con->chunked */&& select_zero_cnt == select_max_retry_time) {
            select_breakout_flag = 1;
            break;
        }
        if (!con->connected && select_zero_cnt == SELECT_RETRY_WHEN_CONNECTING) {
            con->open_fail = 1;
            select_breakout_flag = 1;
            break;
        }
    }

    // select() returns 0 for a long time sometimes, need to breakout.
    if (select_breakout_flag == 1) {
        return CURLERROR(C_ERROR_PERFORM_SELECT_ERROR);
    }

    int msgs_left;
    CURLMsg *msg;
    CURLcode retcode;
    curl_error_code ret = C_ERROR_OK;
    while ((msg = curl_multi_info_read(con->multi_curl, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            int found = 0;
            for (tmp_h = con->curl_handle; tmp_h; tmp_h = tmp_h->next) {
                found = (msg->easy_handle == tmp_h->curl);
                if (found) {
                    break;
                }
            }
            if (!tmp_h) {
                CLOGI("curl_multi_info_read curl not found\n");
            } else {
                CLOGI("[perform done]: completed with status: [%d]\n", msg->data.result);
                if (CURLE_OK != msg->data.result) {
                    tmp_h->perform_error_code = CURLERROR(msg->data.result + C_ERROR_PERFORM_BASE_ERROR);
                    ret = tmp_h->perform_error_code;
                }

                if (CURLE_RECV_ERROR == msg->data.result
                    || CURLE_COULDNT_CONNECT == msg->data.result) {
                    tmp_h->open_quited = 1;
                }

#if 1
                long arg = 0;
                char * argc = NULL;
                retcode = curl_learning_wrapper_get_info(tmp_h, C_INFO_RESPONSE_CODE, 0, &arg);
                if (CURLE_OK == retcode) {
                    CLOGI("[perform done]: response_code: [%ld]\n", arg);
                }
                /*
                if (404 == arg) {
                    ret = C_ERROR_HTTP_404;
                }
                if (retcode == CURLE_PARTIAL_FILE) {
                    ret = C_ERROR_PARTIAL_DATA;
                } else if (retcode == CURLE_OPERATION_TIMEDOUT) {
                    ret = C_ERROR_TRANSFERTIMEOUT;
                }
                retcode = curl_wrapper_get_info(tmp_h, C_INFO_EFFECTIVE_URL, 0, argc);
                if (CURLE_OK == retcode) {
                    CLOGI("uri:[%s], effective_url=[%s]\n", tmp_h->uri, argc);
                }
                */
                retcode = curl_learning_wrapper_get_info(tmp_h, C_INFO_SPEED_DOWNLOAD, 0, &tmp_h->dl_speed);
                if (CURLE_OK == retcode) {
                    CLOGI("[perform done]: This uri's average download speed is: %0.2f kbps\n", (tmp_h->dl_speed * 8) / 1024.00);
                }

                /* just for download speed now, maybe more later */
                if (tmp_h->infonotify) {
                    (*(tmp_h->infonotify))((void *)&tmp_h->dl_speed, NULL);
                }
#endif

            }
        }
    }
    CLOGI("Multi perform return : %d", ret);
    return ret;
}
/**


*/
int curl_learning_wrapper_close(CurlWContext * h)
{
    CLOGI("curl_wrapper_close enter\n");
    int ret = -1;
    if (!h) {
        return ret;
    }
    if (h->multi_curl) {
        curl_multi_cleanup(h->multi_curl);
        h->multi_curl = NULL;
    }
    if (h->curl_handle) {
        curl_learning_wrapper_del_all_curl_handle(h);
    }
    c_aml_free(h);
    h = NULL;
    ret = 0;
    s_LocationNum = 0;
    return ret;
}

/**
  for seek function we have set range for CURLOPT_RANGE;
  http1.1;
*/
int curl_learning_wrapper_seek(CurlWContext * con, CurlWHandle * h, int64_t off, Curl_Data *buf, curl_prot_type flags)
{
    CLOGI("curl_wrapper_seek enter\n");
    int ret = -1;
    if (!con || !h) {
        CLOGE("CURLWHandle invalid\n");
        return ret;
    }
    ret = curl_learning_wrapper_open_cnx(con, h, buf, flags, off);
    if (!ret) {
#if 0
        char range[256];
        snprintf(range, sizeof(range), "%lld-", off);
        if (CURLE_OK != curl_easy_setopt(h->curl, CURLOPT_RANGE, range)) {
            return -1;
        }

#else
        //if (h->chunk_size > 0) {  // not support this when transfer in trunk mode.
          //  ret = curl_easy_setopt(h->curl, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)off);
        //}
#endif
    }
    return ret;
}


int curl_learning_wrapper_get_info_easy(CurlWHandle * h, curl_info cmd, uint32_t flag, int64_t * iinfo, char * cinfo)
{
    CLOGI("curl_wrapper_get_info_easy enter\n");
    if (!h) {
        CLOGE("CurlWHandle invalid\n");
        return -1;
    }
    CURLcode rc;
    CURL *tmp_c = NULL;
    tmp_c = curl_easy_init();
    if (tmp_c) {
        if (cmd == C_INFO_CONTENT_LENGTH_DOWNLOAD) {
            curl_easy_setopt(tmp_c, CURLOPT_URL, h->uri);
            curl_easy_setopt(tmp_c, CURLOPT_NOBODY, 1L);
            curl_easy_setopt(tmp_c, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(tmp_c, CURLOPT_HEADER, 0L);
            rc = curl_easy_perform(tmp_c);
            if (CURLE_OK == rc) {
                //rc = curl_easy_getinfo(tmp_c, CURLINFO_CONTENT_LENGTH_DOWNLOAD, (double *)info);
                double tp;
                rc = curl_easy_getinfo(tmp_c, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &tp);
                *iinfo = (int64_t)tp;
            }
            curl_easy_cleanup(tmp_c);
            tmp_c = NULL;
        }
        if (cmd == C_INFO_CONTENT_TYPE) {
            curl_easy_setopt(tmp_c, CURLOPT_URL, h->uri);
            curl_easy_setopt(tmp_c, CURLOPT_FOLLOWLOCATION, 1L);
            rc = curl_easy_perform(tmp_c);
            if (CURLE_OK == rc) {
                char *tmp_ch = NULL;
                rc = curl_easy_getinfo(tmp_c, CURLINFO_CONTENT_TYPE, &tmp_ch);
                strcpy(cinfo, tmp_ch);
            }
            curl_easy_cleanup(tmp_c);
            tmp_c = NULL;
        }
    }
    return rc;
}

int curl_wrapper_set_to_quit(CurlWContext * con, CurlWHandle * h)
{
    CLOGI("curl_wrapper_set_to_quit enter\n");
    int ret = 0;
    if (!con) {
        CLOGE("CurlWContext invalid\n");
        ret = -1;
        return ret;
    }
    if (!h) {
        con->quited = 1;
    }
    if (con->curl_handle) {
        CurlWHandle * tmp_h = NULL;
        for (tmp_h = con->curl_handle; tmp_h; tmp_h = tmp_h->next) {
            if (h) {
                if (h == tmp_h) {
                    pthread_cond_signal(&tmp_h->pthread_cond);
                    tmp_h->quited = 1;
                    if (tmp_h->curl) {
                        curl_multi_remove_handle(con->multi_curl, tmp_h->curl);
                        //curl_easy_cleanup(tmp_h->curl);
                        //tmp_h->curl = NULL;
                    }
                    ret = curl_learning_wrapper_del_curl_handle(con, h);
                    break;
                }
                continue;
            }
            pthread_cond_signal(&tmp_h->pthread_cond);
            tmp_h->quited = 1;
        }
    }
    return ret;
}

/**


*/
int curl_learning_wrapper_get_info(CurlWHandle * h, curl_info cmd, uint32_t flag, void * info)
{
    if (!h) {
        CLOGE("CurlWHandle invalid\n");
        return -1;
    }
    if (!h->curl) {
        CLOGE("CurlWHandle curl handle not inited\n");
        return -1;
    }
    //CLOGV("curl_wrapper_get_info,  curl_info : [%d]", cmd);
    CURLcode rc;
    switch (cmd) {
    case C_INFO_EFFECTIVE_URL:
        //  last used effective url
        rc = curl_easy_getinfo(h->curl, CURLINFO_EFFECTIVE_URL, (char **)&info);
        break;
    case C_INFO_RESPONSE_CODE:
        // last received HTTP,FTP or SMTP response code
        rc = curl_easy_getinfo(h->curl, CURLINFO_RESPONSE_CODE, (long *)info);
        break;
    case C_INFO_HTTP_CONNECTCODE:
        // last received proxy response code to a connect request
        rc = curl_easy_getinfo(h->curl, CURLINFO_HTTP_CONNECTCODE, (long *)info);
        break;
    case C_INFO_TOTAL_TIME:
        // the total time in seconds for the previous transfer
        rc = curl_easy_getinfo(h->curl, CURLINFO_TOTAL_TIME, (double *)info);
        break;
    case C_INFO_NAMELOOKUP_TIME:
        // from the start until the name resolving was completed, in seconds
        rc = curl_easy_getinfo(h->curl, CURLINFO_NAMELOOKUP_TIME, (double *)info);
        break;
    case C_INFO_CONNECT_TIME:
        // from the start until the connect to the remote host or proxy was completed, in seconds
        rc = curl_easy_getinfo(h->curl, CURLINFO_CONNECT_TIME, (double *)info);
        break;
    case C_INFO_REDIRECT_TIME:
        // contains the complete execution time for multiple redirections, in seconds
        rc = curl_easy_getinfo(h->curl, CURLINFO_REDIRECT_TIME, (double *)info);
        break;
    case C_INFO_REDIRECT_COUNT:
        // the total number of redirections that were actually followed
        rc = curl_easy_getinfo(h->curl, CURLINFO_REDIRECT_COUNT, (long *)info);
        break;
    case C_INFO_REDIRECT_URL:
        // the URL a redirect would take you to
        rc = curl_easy_getinfo(h->curl, CURLINFO_REDIRECT_URL, (char **)&info);
        break;
    case C_INFO_SIZE_DOWNLOAD:
        // the total amount of bytes that were downloaded by the latest transfer
        rc = curl_easy_getinfo(h->curl, CURLINFO_SIZE_DOWNLOAD, (double *)info);
        break;
    case C_INFO_SPEED_DOWNLOAD:
        // the average download speed that measured for the complete download, in bytes/second
        rc = curl_easy_getinfo(h->curl, CURLINFO_SPEED_DOWNLOAD, (double *)info);
        break;
    case C_INFO_HEADER_SIZE:
        // the total size in bytes of all the headers received
        rc = curl_easy_getinfo(h->curl, CURLINFO_HEADER_SIZE, (long *)info);
        break;
    case C_INFO_REQUEST_SIZE:
        // the total size of the issued requests which only for HTTP
        rc = curl_easy_getinfo(h->curl, CURLINFO_REQUEST_SIZE, (long *)info);
        break;
    case C_INFO_CONTENT_LENGTH_DOWNLOAD:
        // the content-length of the download, this returns -1 if the size isn't known
        rc = curl_easy_getinfo(h->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, (double *)info);
        break;
    case C_INFO_CONTENT_TYPE:
        // the content-type of the downloaded object, this returns NULL when unavailable
        rc = curl_easy_getinfo(h->curl, CURLINFO_CONTENT_TYPE, (char **)&info);
        break;
    case C_INFO_NUM_CONNECTS:
        // how many new connections libcurl had to create to achieve the previous transfer, only the successful connects
        rc = curl_easy_getinfo(h->curl, CURLINFO_NUM_CONNECTS, (long *)info);
        break;
    case C_INFO_APPCONNECT_TIME:
        // the time in seconds, took from the start until the SSL/SSH connect/handshake to the remote host was completed
        rc = curl_easy_getinfo(h->curl, CURLINFO_APPCONNECT_TIME, (double *)info);
        break;
    default:
        break;
    }
    return rc;
}


int curl_learning_wrapper_register_notify(CurlWHandle * h, infonotifycallback pfunc)
{
    if (!h) {
        CLOGE("CurlWHandle invalid\n");
        return -1;
    }
    if (pfunc) {
        h->infonotify = pfunc;
    }
    return 0;
}

