
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"

/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	PROCESS* p;

	//my_Add 每次调度睡眠时间sleep_ticks-- 直至 0
	for (p = proc_table; p < proc_table + NR_TASKS; p++) {
        if (p->sleep_ticks > 0) {
            p->sleep_ticks--;
        }
    }
    //end

	int	 greatest_ticks = 0;

	while (!greatest_ticks) {
		for (p = proc_table; p < proc_table+NR_TASKS; p++) {
        	//被阻塞或有睡眠时间,不分配时间片
			if(p->blocked == 1 || p->sleep_ticks > 0){
				continue;
			}
			//end
			if (p->ticks > greatest_ticks) {
				greatest_ticks = p->ticks;
				p_proc_ready = p;
			}
		}
 
		if (!greatest_ticks) {
			for (p = proc_table; p < proc_table+NR_TASKS; p++) {
				//被阻塞或睡眠,不被分配时间片
				if(p->blocked == 1 || p->sleep_ticks > 0){
					continue;
				}
				//end
				p->ticks = p->priority;
			}
		}
	}
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}

PUBLIC void sys_sleep(int milli_seconds)
{
	//毫秒转化为 ticks 数
    p_proc_ready->sleep_ticks = milli_seconds * HZ / 1000;
	schedule();
	return 0;
}

PUBLIC void sys_my_disp(char* str, int color)
{
	disp_color_str(str, color);
	disp_str("");

	//屏幕满后清空屏幕
	if(disp_pos>=80*25*2){
		disp_pos = 0;
		for (int i = 0; i < 80*25; i++) {
			disp_str(" ");
		}
		disp_pos = 0;
	}
}

PUBLIC void sys_my_disp_int(int num)
{
	disp_int(num);
}

PUBLIC void sys_P(SEMAPHORE* s)
{
    s->value--;
    if (s->value < 0) {
        s->list[-s->value-1] = p_proc_ready;//移入等待队列
        p_proc_ready->blocked = 1;//阻塞自己
        schedule();//转向进程调度
    }
}

PUBLIC void sys_V(SEMAPHORE* s)
{
    s->value++;
    if (s->value <= 0) {
        s->list[0]->blocked = 0;//释放自己
        for (int i = 0; i < -s->value; i++) {
            s->list[i] = s->list[i+1]; //后面信号前移一位
        }
        schedule();
    }
}
