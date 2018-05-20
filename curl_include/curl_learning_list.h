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

/* 
    prefetch(x) attempts to pre-emptively get the memory pointed to by address "x" into the CPU L1 cache.  prefetch(x) should not cause any kind of exception, prefetch(0) is 
    specifically ok.

    prefetch() should be defined by the architecture, if not, the #define below provides a no-op define.   
    There are 3 prefetch() macros: 
    prefetch(x)      - prefetches the cacheline at "x" for read

    prefetchw(x)    - prefetches the cacheline at "x" for write 
    spin_lock_prefetch(x) - prefetches the spinlock *x for taking there is also PREFETCH_STRIDE which is the architecure-prefered "lookahead" size for prefetching streamed operations. 
*/

#define prefetch(x) (x)

#ifndef typeof
#define typeof(T)  __typeof__(T)
#endif

/*
   just define a temp typemember __tmptr;
   typemember*tmpptr=ptr;
   __tmptr-offsetof(ptr) that is a type ptr;
   that expression finally define a constainer_of just a 
   type ptr constainer is just a type ptr;
   we have pass a ptr (emmbed in struct typemember)
*/
#define container_of(ptr, type, member) ({\
        const typeof( ((type *)0)->member ) *__tmptr = (ptr);\
        (type *)( (char *)__tmptr - offsetof(type,member) );})

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_first_entry(ptr, type, member)\
    list_entry((ptr)->next, type, member)

#define list_for_each(pos, head) \
    for (pos = (head)->next; prefetch(pos->next), pos != (head); \
            pos = pos->next)
/*
   ptr:the list head;
*/
#define list_last_entry(ptr, type, member) \
    list_entry((ptr)->prev, type, member) 

#define __list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_prev(pos, head) \
     for (pos = (head)->prev; pos != (head); \
         pos = pos->prev)

#define list_for_each_safe(pos, n, head) \
     for (pos = (head)->next, n = pos->next; pos != (head); \
         pos = n, n = pos->next)

/*
     we iterator each member and we have last get a ptr to a struct type
     pos type is a struct;
*/
#define list_for_each_entry(pos, head, member)\
 for (pos = list_entry((head)->next, typeof(*pos), member);\
     &pos->member != (head); \
      pos = list_entry(pos->member.next, typeof(*pos), member))

/*
   list_for_each_entry_reverse
   pos is a pointer to a struct;
*/
#define list_for_each_entry_reverse(pos, head, member)\
    for (pos = list_entry((head)->prev, typeof(*pos), member);\
         &pos->member != (head);\
       pos = list_entry(pos->member.prev, typeof(*pos), member))
/*
     prepare a address;
*/
#define list_prepare_entry(pos, head, member) \
    ((pos) ? : list_entry(head, typeof(*pos), member))

#define list_for_each_entry_continue(pos, head, member)\
    for (pos = list_entry(pos->member.next, typeof(*pos), member);\
       &pos->member != (head);\
       pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_from(pos, head, member)\
     for (; &pos->member != (head);\
       pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)\
    for (pos = list_entry((head)->next, typeof(*pos), member), \
         n = list_entry(pos->member.next, typeof(*pos), member);\
         &pos->member != (head);\
         pos = n, n = list_entry(n->member.next, typeof(*n), member))

#define list_for_each_entry_safe_continue(pos, n, head, member) \
    for (pos = list_entry(pos->member.next, typeof(*pos), member), \
        n = list_entry(pos->member.next, typeof(*pos), member);\
         &pos->member != (head);\
        pos = n, n = list_entry(n->member.next, typeof(*n), member))

#define list_for_each_entry_safe_from(pos, n, head, member) \
 for (n = list_entry(pos->member.next, typeof(*pos), member);\
      &pos->member != (head);\
      pos = n, n = list_entry(n->member.next, typeof(*n), member))

/**
 * list_for_each_entry_safe_reverse
 * @pos: the type * to use as a loop cursor.
 * @n: another type * to use as temporary storage
 * @head: the head for your list.
 * @member:the name of the list_struct within the struct.
 *
 * Iterate backwards over list of given type, safe against removal
 * of list entry.
 */
#define list_for_each_entry_safe_reverse(pos, n, head, member)\
 for (pos = list_entry((head)->prev, typeof(*pos), member),\
 n = list_entry(pos->member.prev, typeof(*pos), member);\
     &pos->member != (head); \
     pos = n, n = list_entry(n->member.prev, typeof(*n), member))
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
#ifndef CONFIG_DEBUG_LIST
static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}
#else
extern void INIT_LIST_HEAD(struct list_head *list);
#endif
/*
  inner new add listhead;
  one for new;
  one for pre;
  one for next;
 ----->head->prev->next=new2add->next;
 next->a=c;
 next->a=b;´íÎó;
===>ÍÆ²»³öc=b;
3412;
*/
#ifndef CONFIG_DEBUG_LIST
static inline void inner_list_add(struct list_head *new2add,
             struct list_head *prev,
             struct list_head *next)
{
      next->prev=new2add;
      new2add->next=next;
      new2add->prev=prev;
      prev->next=new2add;
}
#else
extern void inner_list_add(struct list_head *new,
                              struct list_head *prev,
                              struct list_head *next);
#endif


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
#ifndef CONFIG_DEBUG_LIST
static inline void inner_list_del(struct list_head * prev, struct list_head * next)
{
    next->prev = prev;
    prev->next = next;
}
#else
extern void inner_list_del(struct list_head * prev, struct list_head * next);
#endif

static inline void inner_list_del_entry(struct list_head*entry){
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

/*
  no other cpu is modifying the list node;
*/
static inline int list_empty_careful(const struct list_head *head)
{
	struct list_head *next = head->next;
	return (next == head) && (next == head->prev);
}

/*
  to delete all head in list;
  different frome sgi stl that list have no node;
  what we need to learn is about the splice process;
  we can see that point:
     first->prev=head;
	 head->next=fist;
	 last->next=at;
	 at->prev=last; 
	 1234;
*/
#ifndef CONFIG_DEBUG_LIST
static inline void inner_list_splice(const struct list_head *list,
                                 struct list_head *prev,
                                 struct list_head *next)
{
        struct list_head *first = list->next;
        struct list_head *last = list->prev;

        first->prev = prev;
        prev->next = first;

        last->next = next;
        next->prev = last;
}
#else
extern void inner_list_splice(const struct list_head *list,
                                 struct list_head *prev,
                                 struct list_head *next);
#endif



static inline void list_splice(struct list_head *list, struct list_head *head)
{
     if (!list_empty(list))
        inner_list_splice(list, head,head->next);
}


/*
	list_splice_init for that at last the list can be a emptry;
*/
static inline void list_splice_init(struct list_head *list,
          struct list_head *head)
{
    if (!list_empty(list)) {
        inner_list_splice(list,head,head->next);
        INIT_LIST_HEAD(list);
    }
}

static inline void list_splice_tail(struct list_head *list,
                                struct list_head *head)
{
        if (!list_empty(list))
                inner_list_splice(list, head->prev, head);
}

static inline void list_splice_tail_init(struct list_head *list,
                                struct list_head *head)
{
        if (!list_empty(list)) {
                inner_list_splice(list, head->prev, head);
				INIT_LIST_HEAD(list);
        }
}

#ifndef CONFIG_DEBUG_LIST
/*
  what we do is just replace the old by new;
  to replace a node;
  four lines;3412;
*/
static inline void list_replace(struct list_head *old,
                                struct list_head *new)
{
        new->next = old->next;
        new->next->prev = new;
        new->prev = old->prev;
        new->prev->next = new;
}
#else
extern void list_replace(struct list_head *old,
                                struct list_head *new);
#endif

/*
   we just replace a node by new and set old a empty list node;
*/
static inline void list_replace_init(struct list_head *old,
                                        struct list_head *new)
{
        list_replace(old, new);
        INIT_LIST_HEAD(old);
}

/*
   list move is just a delete a entry;
   and add a head in the stack list;
*/
static inline void list_move(struct list_head *list, struct list_head *head)
{
        inner_list_del_entry(list);
        list_add(list, head);
}


/*
   what we do is just delete a list node;
   and add a head in the tail;
*/
static inline void list_move_tail(struct list_head *list,
                                  struct list_head *head)
{
        inner_list_del_entry(list);
        list_add_tail(list, head);
}


/*
  what we do is just do rotate;we add fistt to the tail;
  then we get the next;
*/
static inline void list_rotate_left(struct list_head *head)
{
        struct list_head *first;

        if (!list_empty(head)) {
                first = head->next;
                list_move_tail(first, head);
        }
}

/*
  what we need to do is just certain that list_head cannot be modify;
  that the point to a data; data cannot be change;
  here is that the list->prev and next cannt be change;
*/
static inline int list_is_last(const struct list_head *list,
                                const struct list_head *head)
{
     return list->next == head;
}

static inline int list_is_singular(const struct list_head *head)
{
        return !list_empty(head) && (head->next == head->prev);
}

#ifndef CONFIG_DEBUG_LIST
/*
   we cut a list from a entry; 
   and last list and head is two list;
*/
static inline void inner_list_cut_position(struct list_head *list,
                struct list_head *head, struct list_head *entry)
{
        struct list_head *new_first = entry->next;
        list->next = head->next;
        list->next->prev = list;
        list->prev = entry;
        entry->next = list;
        head->next = new_first;
        new_first->prev = head;
}
#else
extern void inner_list_cut_position(struct list_head *list,
                struct list_head *head, struct list_head *entry);
#endif

static inline void list_cut_position(struct list_head *list,
                struct list_head *head, struct list_head *entry)
{
        if (list_empty(head))
             return;
        if (list_is_singular(head) && (head->next != entry && head != entry))
             return;
        if (entry == head)
             INIT_LIST_HEAD(list);
        else
             inner_list_cut_position(list, head, entry);
}


#ifdef __cplusplus
}
#endif

#endif
