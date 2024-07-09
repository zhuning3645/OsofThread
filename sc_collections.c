#include <stddef.h>
#include "sc_collections.h"


// 初始化链表
void sc_list_init(struct sc_list_head *list) {
    list->next = list;
    list->prev = list;
}

// 添加节点到链表头
void sc_list_add(struct sc_list_head *new, struct sc_list_head *head) {
    new->next = head->next;
    new->prev = head;
    head->next->prev = new;
    head->next = new;
}

// 添加节点到链表尾
void sc_list_add_tail(struct sc_list_head *new, struct sc_list_head *head) {
    new->next = head;
    new->prev = head->prev;
    head->prev->next = new;
    head->prev = new;
}

// 从链表中删除节点
void sc_list_remove(struct sc_list_head *entry) {
    if (entry == NULL) {
        return;
    }

    if (entry->prev != NULL) {
        entry->prev->next = entry->next;
    }
    if (entry->next != NULL) {
        entry->next->prev = entry->prev;
    }

    entry->prev = NULL;
    entry->next = NULL;
}

// 检查链表是否为空
int sc_list_empty(const struct sc_list_head *head) {
    return head->next == head;
}

void sc_list_insert_end(struct sc_list_head *head, struct sc_list_head *node) {
    struct sc_list_head *last = head->prev;

    last->next = node;
    node->prev = last;
    node->next = head;
    head->prev = node;
}


