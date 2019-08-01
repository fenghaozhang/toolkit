#ifndef _SRC_BASE_LIST_H
#define _SRC_BASE_LIST_H

/**
 * Refer to list.h in linux kernel
 */

// From kernel list
typedef struct list_t list_t;

struct list_t {
    list_t              *next, *prev;
};

#define EASY_LIST_HEAD_INIT(name) {&(name), &(name)}
#define list_init(ptr) do {                     \
        (ptr)->next = (ptr);                    \
        (ptr)->prev = (ptr);                    \
    } while (0)

static inline void __list_add(list_t *list, list_t *prev, list_t *next)
{
    next->prev = list;
    list->next = next;
    list->prev = prev;
    prev->next = list;
}

static inline void __list_del(list_t *prev, list_t *next)
{
    next->prev = prev;
    prev->next = next;
}

// List head to add it after
static inline void list_add_head(list_t *list, list_t *head)
{
    __list_add(list, head, head->next);
}

// List head to add it before
static inline void list_add_tail(list_t *list, list_t *head)
{
    __list_add(list, head->prev, head);
}

// Deletes entry from list
static inline void list_del(list_t *entry)
{
    __list_del(entry->prev, entry->next);
    list_init(entry);
}

// Tests whether a list is empty
static inline int list_empty(const list_t *head)
{
    return (head->next == head);
}

// Move list to new_list
static inline void list_movelist(list_t *list, list_t *new_list)
{
    if (!list_empty(list)) {
        new_list->prev = list->prev;
        new_list->next = list->next;
        new_list->prev->next = new_list;
        new_list->next->prev = new_list;
        list_init(list);
    } else {
        list_init(new_list);
    }
}

// Join list to head
static inline void list_join(list_t *list, list_t *head)
{
    if (!list_empty(list)) {
        list_t             *first = list->next;
        list_t             *last = list->prev;
        list_t             *at = head->prev;

        first->prev = at;
        at->next = first;
        last->next = head;
        head->prev = last;
    }
}

#define list_entry(ptr, type, mbr) ({(type*)((char*)(ptr)-offsetof(type, mbr));}) // NOLINT

// Get last obj
#define list_get_last(list, type, mbr) (list_empty(list) ? NULL : list_entry((list)->prev, type, mbr))

// Get first obj
#define list_get_first(list, type, mbr) (list_empty(list) ? NULL : list_entry((list)->next, type, mbr))

#define list_for_each_entry(it, hdr, mbr)                                                                       \
    for (it = list_entry((hdr)->next, __typeof__(*it), mbr);                                                     \
        (hdr) != &it->mbr; it = list_entry(it->mbr.next, __typeof__(*it), mbr))

#define list_for_each_entry_reverse(it, hdr, mbr)                                                               \
    for (it = list_entry((hdr)->prev, __typeof__(*it), mbr);                                                     \
        (hdr) != &it->mbr; it = list_entry(it->mbr.prev, __typeof__(*it), mbr))

#define list_for_each_entry_safe(it, n, hdr, mbr)                                                               \
    for (it = list_entry((hdr)->next, __typeof__(*it), mbr), n = list_entry(it->mbr.next, __typeof__(*it), mbr);   \
        (hdr) != &it->mbr; it = n, n = list_entry(n->mbr.next, __typeof__(*n), mbr))

#define list_for_each_entry_safe_reverse(it, n, hdr, mbr)                                                       \
    for (it = list_entry((hdr)->prev, __typeof__(*it), mbr), n = list_entry(it->mbr.prev, __typeof__(*it), mbr);   \
        (hdr) != &it->mbr; it = n, n = list_entry(n->mbr.prev, __typeof__(*n), mbr))

#endif  // _SRC_BASE_LIST_H
