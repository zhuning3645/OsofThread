#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>

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
		printf("task %s: %d\n", ta->name, i);
		//让出调度权
		scheduler_relinquish();
	}
	free(ta);
}

//创建任务的函数
void create_test_task(char *name, int iters)
{
	struct tester_args *ta = malloc(sizeof(*ta));
	ta->name = name;
	ta->iters = iters;
	//向调度器新注册一个新任务
	scheduler_create_task(tester, ta);
}

int main(int argc, char **argv)
{
	scheduler_init();
	create_test_task("first", 5);
	create_test_task("second", 2);
	scheduler_run();
	printf("Finished running all tasks!\n");
	return EXIT_SUCCESS;
}
