#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>

#include "sc_collections.h"
#include "scheduler.h"

struct tester_args {
	char *name;
	int iters;
};

//定义任务处理函数
void tester(void *arg)
{
	int i;
	struct tester_args *ta = (struct tester_args *)arg;
	for (i = 0; i < ta->iters; i++) {
		// 在访问共享资源前加锁
		mutex_lock(&ta->mutex);
		printf("task %s: %d\n", ta->name, i);
		// 访问完成后解锁
		mutex_unlock(&ta->mutex);

		//让出调度权
		scheduler_relinquish();
	}
	free(ta);
}

//创建任务的函数
void create_test_task(char *name, int iters, struct mutex *mutex)
{
	struct tester_args *ta = malloc(sizeof(*ta));
	ta->name = name;
	ta->iters = iters;
	ta->mutex = mutex; // 添加互斥锁指针到任务参数
	//向调度器新注册一个新任务
	scheduler_create_task(tester, ta);
}

// 已经实现了mutex的初始化、锁定、解锁函数
void mutex_init(struct mutex *mutex);
void mutex_lock(struct mutex *mutex);
void mutex_unlock(struct mutex *mutex);

int main(int argc, char **argv)
{
	(void)argc; 
    (void)argv;
	scheduler_init();
	// 创建一个全局的互斥锁
	struct mutex global_mutex;
	mutex_init(&global_mutex);

	// 创建第一个测试任务，带互斥锁保护
	create_test_task_with_mutex("first", 5, &global_mutex);

	// 创建第二个测试任务，同样带互斥锁保护
	create_test_task_with_mutex("second", 2, &global_mutex);
	scheduler_run();
	printf("Finished running all tasks!\n");
	return EXIT_SUCCESS;
}
