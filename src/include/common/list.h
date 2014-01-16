/**************************************************************************//**
            Copyright 2013 Freescale Semiconductor, Inc.

 @File          list.h

 @Description   External prototypes for list
*//***************************************************************************/

#ifndef __FSL_LIST_H
#define __FSL_LIST_H

#include "common/types.h"


/**************************************************************************//**
 @Group         fsl_lib_g   Utility Library Application Programming Interface

 @Description   External routines.

 @{
*//***************************************************************************/

/**************************************************************************//**
 @Group         fsl_lib_lst_g List API

 @Description   List module functions,definitions and enums.

 @{
*//***************************************************************************/

/**************************************************************************//**
 @Description   List structure.
*//***************************************************************************/
typedef struct list {
    struct list *next;  /**< A pointer to the next list object     */
    struct list *prev;  /**< A pointer to the previous list object */
} list_t;


/**************************************************************************//**
 @Function      LIST_FIRST/LIST_LAST/LIST_NEXT/LIST_PREV

 @Description   Macro to get first/last/next/previous entry in a list.

 @Param[in]     lst - A pointer to a list.
*//***************************************************************************/
#define LIST_FIRST(lst) (lst)->next
#define LIST_LAST(lst)  (lst)->prev
#define LIST_NEXT       LIST_FIRST
#define LIST_PREV       LIST_LAST

/**************************************************************************//**
 @Function      LIST_INIT

 @Description   Macro for initialization of a list struct.

 @Param[in]     lst - The t_List object to initialize.
*//***************************************************************************/
#define LIST_INIT(lst) {&(lst), &(lst)}

/**************************************************************************//**
 @Function      LIST

 @Description   Macro to declare of a list.

 @Param[in]     listName - The list object name.
*//***************************************************************************/
#define LIST(list_name) list_t list_name = LIST_INIT(list_name)

/**************************************************************************//**
 @Function      INIT_LIST

 @Description   Macro to initialize a list pointer.

 @Param[in]     lst - The list pointer.
*//***************************************************************************/
#define INIT_LIST(lst)   LIST_FIRST(lst) = LIST_LAST(lst) = (lst)

/**************************************************************************//**
 @Function      LIST_OBJECT

 @Description   Macro to get the struct (object) for this entry.

 @Param[in]     type   - The type of the struct (object) this list is embedded in.
 @Param[in]     member - The name of the t_List object within the struct.

 @Return        The structure pointer for this entry.
*//***************************************************************************/
#define MEMBER_OFFSET(type, member) (PTR_TO_UINT(&((type *)0)->member))
#define LIST_OBJECT(lst, type, member) \
    ((type *)((char *)(lst)-MEMBER_OFFSET(type, member)))

/**************************************************************************//**
 @Function      LIST_FOR_EACH

 @Description   Macro to iterate over a list.

 @Param[in]     pos  - A pointer to a list to use as a loop counter.
 @Param[in]     head - A pointer to the head for your list pointer.

 @Cautions      You can't delete items with this routine.
                For deletion use LIST_FOR_EACH_SAFE().
*//***************************************************************************/
#define LIST_FOR_EACH(pos, head) \
    for (pos = LIST_FIRST(head); pos != (head); pos = LIST_NEXT(pos))

/**************************************************************************//**
 @Function      LIST_FOR_EACH_SAFE

 @Description   Macro to iterate over a list safe against removal of list entry.

 @Param[in]     pos     - A pointer to a list to use as a loop counter.
 @Param[in]     tmp_pos - Another pointer to a list to use as temporary storage.
 @Param[in]     head    - A pointer to the head for your list pointer.
*//***************************************************************************/
#define LIST_FOR_EACH_SAFE(pos, tmp_pos, head)              \
    for (pos = LIST_FIRST(head), tmp_pos = LIST_FIRST(pos); \
         pos != (head);                                     \
         pos = tmp_pos, tmp_pos = LIST_NEXT(pos))

/**************************************************************************//**
 @Function      LIST_FOR_EACH_OBJECT_SAFE

 @Description   Macro to iterate over list of given type safely.

 @Param[in]     pos     - A pointer to a list to use as a loop counter.
 @Param[in]     tmp_pos - Another pointer to a list to use as temporary storage.
 @Param[in]     type    - The type of the struct this is embedded in.
 @Param[in]     head    - A pointer to the head for your list pointer.
 @Param[in]     member  - The name of the list_struct within the struct.

 @Cautions      You can't delete items with this routine.
                For deletion use LIST_FOR_EACH_SAFE().
*//***************************************************************************/
#define LIST_FOR_EACH_OBJECT_SAFE(pos, tmp_pos, head, type, member)    \
    for (pos = LIST_OBJECT(LIST_FIRST(head), type, member),            \
         tmp_pos = LIST_OBJECT(LIST_FIRST(&pos->member), type, member);\
         &pos->member != (head);                                       \
         pos = tmp_pos,                                                \
         tmp_pos = LIST_OBJECT(LIST_FIRST(&pos->member), type, member))

/**************************************************************************//**
 @Function      LIST_FOR_EACH_OBJECT

 @Description   Macro to iterate over list of given type.

 @Param[in]     pos  - A pointer to a list to use as a loop counter.
 @Param[in]     type   - The type of the struct this is embedded in.
 @Param[in]     head - A pointer to the head for your list pointer.
 @Param[in]     member - The name of the list_struct within the struct.

 @Cautions      You can't delete items with this routine.
                For deletion use LIST_FOR_EACH_SAFE().
*//***************************************************************************/
#define LIST_FOR_EACH_OBJECT(pos, type, head, member)                  \
    for (pos = LIST_OBJECT(LIST_FIRST(head), type, member);            \
         &pos->member != (head);                                       \
         pos = LIST_OBJECT(LIST_FIRST(&(pos->member)), type, member))


/**************************************************************************//**
 @Function      list_add

 @Description   Add a new entry to a list.

                Insert a new entry after the specified head.
                This is good for implementing stacks.

 @Param[in]     p_new  - A pointer to a new list entry to be added.
 @Param[in]     head - A pointer to a list head to add it after.

 @Return        none.
*//***************************************************************************/
static inline void list_add(list_t *p_new, list_t *head)
{
    LIST_PREV(LIST_NEXT(head)) = p_new;
    LIST_NEXT(p_new)             = LIST_NEXT(head);
    LIST_PREV(p_new)             = head;
    LIST_NEXT(head)            = p_new;
}

/**************************************************************************//**
 @Function      list_add_to_tail

 @Description   Add a new entry to a list.

                Insert a new entry before the specified head.
                This is useful for implementing queues.

 @Param[in]     p_new  - A pointer to a new list entry to be added.
 @Param[in]     head - A pointer to a list head to add it before.

 @Return        none.
*//***************************************************************************/
static inline void list_add_to_tail(list_t *p_new, list_t *head)
{
    LIST_NEXT(LIST_PREV(head)) = p_new;
    LIST_PREV(p_new)             = LIST_PREV(head);
    LIST_NEXT(p_new)             = head;
    LIST_PREV(head)            = p_new;
}

/**************************************************************************//**
 @Function      list_del

 @Description   Deletes entry from a list.

 @Param[in]     entry - A pointer to the element to delete from the list.

 @Return        none.

 @Cautions      LIST_is_Empty() on entry does not return true after this,
                the entry is in an undefined state.
*//***************************************************************************/
static inline void list_del(list_t *entry)
{
    LIST_PREV(LIST_NEXT(entry)) = LIST_PREV(entry);
    LIST_NEXT(LIST_PREV(entry)) = LIST_NEXT(entry);
}

/**************************************************************************//**
 @Function      list_del_and_init

 @Description   Deletes entry from list and reinitialize it.

 @Param[in]     entry - A pointer to the element to delete from the list.

 @Return        none.
*//***************************************************************************/
static inline void list_del_and_init(list_t *entry)
{
    list_del(entry);
    INIT_LIST(entry);
}

/**************************************************************************//**
 @Function      list_move

 @Description   Delete from one list and add as another's head.

 @Param[in]     entry - A pointer to the list entry to move.
 @Param[in]     head  - A pointer to the list head that will precede our entry.

 @Return        none.
*//***************************************************************************/
static inline void list_move(list_t *entry, list_t *head)
{
    list_del(entry);
    list_add(entry, head);
}

/**************************************************************************//**
 @Function      list_move_to_tail

 @Description   Delete from one list and add as another's tail.

 @Param[in]     entry - A pointer to the entry to move.
 @Param[in]     head  - A pointer to the list head that will follow our entry.

 @Return        none.
*//***************************************************************************/
static inline void list_move_to_tail(list_t *entry, list_t *head)
{
    list_del(entry);
    list_add_to_tail(entry, head);
}

/**************************************************************************//**
 @Function      list_is_empty

 @Description   Tests whether a list is empty.

 @Param[in]     lst - A pointer to the list to test.

 @Return        1 if the list is empty, 0 otherwise.
*//***************************************************************************/
static inline int list_is_empty(list_t *lst)
{
    return (LIST_FIRST(lst) == lst);
}

/**************************************************************************//**
 @Function      list_append

 @Description   Join two lists.

 @Param[in]     new_lst - A pointer to the new list to add.
 @Param[in]     head    - A pointer to the place to add it in the first list.

 @Return        none.
*//***************************************************************************/
void list_append(list_t *new_lst, list_t *head);

/**************************************************************************//**
 @Function      list_num_of_objs

 @Description   Counts number of objects in the list

 @Param[in]     lst - A pointer to the list which objects are to be counted.

 @Return        Number of objects in the list.
*//***************************************************************************/
int list_num_of_objs(list_t *lst);

/** @} */ /* end of fsl_lib_lst_g group */
/** @} */ /* end of fsl_lib_g group */


#endif /* __FSL_LIST_H */

