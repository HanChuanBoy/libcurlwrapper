#ifndef CURL_LIST_HEADERS_H
#define CURL_LIST_HEADERS_H
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
extern "C" {
#endif

#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)

/*
  use in container of; 
  offsetof just find a member offset relative by type;
  show convert just need by 0-->pointer a TYPE;
*/
#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) (&((TYPE *)0)->MEMBER))
#endif

#define prefetch(x) (x)

#ifndef typeof
#define typeof(T)	__typeof__(T)
#endif

/*
   just define a temp typemember __tmptr;
   typemember*tmpptr=ptr;
   __tmptr-offsetof(ptr) that is a type ptr;
   that expression finally define a constainer_of just a 
   type ptr constainer is just a type ptr;
   we have pass a ptr (emmbed in struct typemember)
*/
#define container_of(ptr, type, member) ({			\
        const typeof( ((type *)0)->member ) *__tmptr = (ptr);	\
        (type *)( (char *)__tmptr - offsetof(type,member) );})

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

#define list_for_each(pos, head) \
    for (pos = (head)->next; prefetch(pos->next), pos != (head); \
            pos = pos->next)


struct list_head {
    struct list_head *next;
    struct list_head *prev;
};

/*
  input a list_head variable;
*/

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

/*
 define a way of init list; 
 the step is about LIST_HEAD_INIT--->INIT_LIST_HEAD;
*/
static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}
/*
  inner new add listhead;
  one for new;
  one for pre;
  one for next;
 ----->head->prev->next=new2add->next;
 next->a=c;
 next->a=b;´íÎó;
===>ÍÆ²»³öc=b;
*/
static inline void inner_list_add(struct list_head *new2add,
             struct list_head *prev,
             struct list_head *next)
{
      next->prev=new2add;
      new2add->next=next;
      new2add->prev=prev;
      prev->next=new2add;
}
/*
   input a new listhead,and a head for list;
   insert a listhead in a listhead_ptr;
   is good for stack;
*/
static inline void list_add(struct list_head *new2add, struct list_head *head)
{
	inner_list_add(new2add, head, head->next);
}

/*
   is good for queue;
*/
static inline void list_add_tail(struct list_head *new2add, struct list_head *head)
{
     inner_list_add(new2add, head->prev, head);
}

/*
  delete a node that between prev and next;
*/
static inline void inner_list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list_head*entry){
	 inner_list_del(entry->prev, entry->next);
	 entry->next = (struct list_head*)LIST_POISON1;
	 entry->prev = (struct list_head *)LIST_POISON2;
}

static inline void list_del_init(struct list_head *entry)
{
    inner_list_del(entry->prev, entry->next);
    INIT_LIST_HEAD(entry);
}

static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}




