#include <stdio.h>
#include <stdlib.h>

#include "curl_learning_list.h"
#include "curl_aml_common.h"

struct kool_list{
    int to;
    struct list_head list;
    int from;
};

int main(int argc, char **argv){

    struct kool_list *tmp;
    struct list_head *pos, *q;
    unsigned int i;

    struct kool_list*mylist=(struct kool_list *)c_aml_mallocz(sizeof(struct kool_list));
    printf("mylist add to= %d from= %d\n", mylist->to, mylist->from);
    INIT_LIST_HEAD(&mylist->list);
	
    /* or you could have declared this with the following macro
     * LIST_HEAD(mylist); which declares and initializes the list
     */

    /* adding elements to mylist */
    for(i=5; i!=0; --i){
        tmp= (struct kool_list *)c_aml_mallocz(sizeof(struct kool_list));
        printf("enter to and from:");
        scanf("%d %d", &tmp->to, &tmp->from);
        list_add_tail(&(tmp->list), &(mylist->list));
    }
    printf("\n");
    printf("traversing the list using list_for_each()\n");
    list_for_each(pos, &mylist->list){
         tmp= list_entry(pos, struct kool_list, list);
         printf("to= %d from= %d\n", tmp->to, tmp->from);

    }
    printf("\n");
    printf("traversing the list using list_for_each_entry()\n");
    list_for_each_entry(tmp, &mylist->list, list){
         printf("to= %d from= %d\n", tmp->to, tmp->from);
    }
    printf("last traverse to= %d from= %d\n", tmp->to, tmp->from);
    list_for_each_entry(tmp,&mylist->list,list){
        if(tmp->to == 2)
            break;
    }
    printf("to you= %d from %d\n",tmp->to,tmp->from);
    printf("\n");
    printf("deleting the list using list_for_each_safe()\n");
    list_for_each_safe(pos, q, &mylist->list){
         tmp= list_entry(pos, struct kool_list, list);
         printf("freeing item to= %d from= %d\n", tmp->to, tmp->from);
         inner_list_del_entry(pos);
         free(tmp);
    }
    return 0;
}