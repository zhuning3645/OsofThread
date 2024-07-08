#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <ucontext.h>

#include "sc_collections.h"

struct task {
	/*
	 * Record the status of this task
	 */
	enum {
		ST_CREATED,
		ST_RUNNING,
		ST_WAITING,
	} status;

	int id;

	/*
	 * For tasks in the ST_RUNNING state, this is where to longjmp back to
	 * in order to resume their execution.
	 */
	ucontext_t context;

	/*
	 * Function and argument to call on startup.
	 */
	void (*func)(void*);
	void *arg;

	/*
	 * Link all tasks together for the scheduler.
	 */
	struct sc_list_head task_list;

	void *stack_bottom;
	void *stack_top;
	int stack_size;
};

enum {
	INIT=0,
	SCHEDULE,
	EXIT_TASK,
};

struct scheduler_private {
	/*
	 * Where to jump back to perform scheduler actions
	 */
	ucontext_t context;

	/*
	 * The current task
	 */
	struct task *current;

	/*
	 * A list of all tasks.
	 */
	struct sc_list_head task_list;
} priv;

//初始化调度权，清空任务列表，设置当前任务为null
void scheduler_init(void)
{
	priv.current = NULL;
	sc_list_init(&priv.task_list);
}

//创建新任务，分配内存，初始化任务状态和属性，将其加入任务列表
void scheduler_create_task(void (*func)(void *), void *arg)
{
	static int id = 1;
	struct task *task = malloc(sizeof(*task));
	task->status = ST_CREATED;
	task->func = func;
	task->arg = arg;
	task->id = id++;
	task->stack_size = 16 * 1024;
	task->stack_bottom = malloc(task->stack_size);
	task->stack_top = task->stack_bottom + task->stack_size;
	sc_list_insert_end(&priv.task_list, &task->task_list);
}

//退出当前任务
void scheduler_exit_current_task(void)
{
	struct task *task = priv.current;
	//任务列表中移除当前任务
	sc_list_remove(&task->task_list);
	/* Would love to free the task... but if we do, we won't have a
	 * stack anymore, which would really put a damper on things.
	 * Let's defer that until we longjmp back into the old stack */
	//通过Longjmp跳回调度器，并且把返回值设置为EXIT_TASK
	// 保存当前任务的上下文并切换到调度器上下文
    // 使用 swapcontext 来实现上下文切换
    if (swapcontext(&task->context, &priv.scheduler_context) == -1) {
        perror("swapcontext");
        exit(EXIT_FAILURE);
    }
	/* NO RETURN */
}

//选择下一个要进行调度的任务
static struct task *scheduler_choose_task(void)
{
	struct task *task;

	sc_list_for_each_entry(task, &priv.task_list, task_list, struct task)
	{
		if (task->status == ST_RUNNING || task->status == ST_CREATED) {
			/* We'll pick this one, but first we should move it to
			 * the back to ensure that we pick a new task next
			 * time. Note that this is only safe because we're
			 * exiting the loop now. If we continued looping we
			 * could have trouble due to modifying the linked list.
			 */
			sc_list_remove(&task->task_list);
			sc_list_insert_end(&priv.task_list, &task->task_list);
			return task;
		}
	}

	return NULL;
}

//核心调度逻辑
static void schedule(void)
{
	struct task *next = scheduler_choose_task();

	if (!next) {
		return;
	}

	priv.current = next;
	if (next->status == ST_CREATED) {
		/*
		 * This task has not been started yet. Assign a new stack
		 * pointer, run the task, and exit it at the end.
		 * 初始化并运行新的任务，设置任务的上下文
		 */
		if (getcontext(&next->context) == -1) {
            perror("getcontext");
            exit(EXIT_FAILURE);
        }
		// 设置任务函数
        makecontext(&next->context, (void (*)())next->func, 1, next->arg);

		/*
		 * Run the task function
		 */
		next->status = ST_RUNNING;

		/*
		 * The stack pointer should be back where we set it. Returning would be
		 * a very, very bad idea. Let's instead exit
		 */
		if (setcontext(&next->context) == -1) {
            perror("setcontext");
            exit(EXIT_FAILURE);
        }
	} else {
		//需要用ucontext()来实现
		//通过longjmp跳转到任务的jmp_buf，恢复之前的任务
		//使用swapcontext恢复之前的任务
		if (swapcontext(&priv.current->context, &next->context) == -1) {
            perror("swapcontext");
            exit(EXIT_FAILURE);
        }
	}
	/* NO RETURN */
}

//主动放弃cpu，保存当前任务状态，跳转回调度器
void scheduler_relinquish(void)
{
	/* if (setjmp(priv.current->buf)) {
		return;
	} else {
		//需要用ucontext()来实现
		longjmp(priv.buf, SCHEDULE);
	} */
	// 获取当前任务的上下文并保存
    if (getcontext(&priv.current->context) == 0) {
        // 保存当前任务上下文成功，进行上下文切换
        // 保存调度器的上下文并切换到调度器上下文
        if (swapcontext(&priv.current->context, &priv.scheduler_context) == -1) {
            perror("swapcontext");
            exit(EXIT_FAILURE);
        }
    } else {
        // 任务上下文被恢复时的处理，这里直接返回
        return;
    }
}

//释放当前任务资源
static void scheduler_free_current_task(void)
{
	struct task *task = priv.current;
	priv.current = NULL;
	free(task->stack_bottom);
	free(task);
}

//启动调度器循环
void scheduler_run(void)
{
	if(getcontext(&priv.context) == 0){
		//初次调用getcontext返回0，相当于INIT状态
		priv.context.uc_stack.ss_sp = malloc(SIGSTKSZ);
		priv.text.uc_stack.ss_size = SIGSTKSZ;
        priv.context.uc_link = &main_context; 
		// 返回上下文
		// 设置要执行的函数和参数，这里简化为 schedule
        makecontext(&priv.context, (void (*)())schedule, 0);

        // 切换到调度器上下文，相当于 setjmp 之后进入 SCHEDULE
        swapcontext(&main_context, &priv.context);
		} else {
        // 后续从 setcontext 恢复时的处理
        int status = main_context.uc_mcontext.gregs[REG_RDI];
        switch (status) {
            case EXIT_TASK:
                scheduler_free_current_task();
                // 没有 break, 跌入到 INIT 继续执行
            case INIT:
            case SCHEDULE:
                // 重新调度，类似 setjmp 的 case 逻辑
                schedule();
                // 如果 schedule 返回，则直接退出
                return;
            default:
                fprintf(stderr, "调度出错！\n");
                return;
        }
	}

}
