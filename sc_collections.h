#ifndef SC_COLLECTIONS_H
#define SC_COLLECTIONS_H

struct sc_list_head {
    // 定义结构体成员
    struct sc_list_head *next, *prev;
};

// 初始化链表头
#define SC_LIST_HEAD_INIT(name) { &(name), &(name) }

// 静态初始化链表头
#define SC_LIST_HEAD(name) \
    struct sc_list_head name = SC_LIST_HEAD_INIT(name)

//初始化链表
void sc_list_init(struct sc_list_head *list);

// 添加节点到链表头
void sc_list_add(struct sc_list_head *new, struct sc_list_head *head);

// 添加节点到链表尾
void sc_list_add_tail(struct sc_list_head *new, struct sc_list_head *head);

// 从链表中删除节点
void sc_list_remove(struct sc_list_head *entry);

// 检查链表是否为空
int sc_list_empty(const struct sc_list_head *head);

void sc_list_insert_end(struct sc_list_head *head, struct sc_list_head *node);

// 遍历链表
#define sc_list_for_each_entry(pos, head, member, type) \
    for (pos = (type *)((char *)(head)->next - offsetof(type, member)); \
         &pos->member != (head); \
         pos = (type *)((char *)pos->member.next - offsetof(type, member)))

#endif // SC_COLLECTIONS_H