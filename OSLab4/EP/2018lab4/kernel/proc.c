
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
    int  greatest_ticks = 0;
    for (p = proc_table; p < proc_table + NR_TASKS; p++) {
        if (p->sleep_ticks > 0) {
            p->sleep_ticks--;
        }
    }
    while (!greatest_ticks) {
        for (p = proc_table; p < proc_table+NR_TASKS; p++) {
            if(p->blocked || p->sleep_ticks)
                continue;
            if (p->ticks > greatest_ticks) {
                greatest_ticks = p->ticks;
                p_proc_ready = p;
            }
        }
        if (!greatest_ticks) {
            for (p = proc_table; p < proc_table+NR_TASKS; p++) {
                 if(p->blocked || p->sleep_ticks)
                    continue;
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

PUBLIC void sys_process_sleep(int mill_seconds)
{
    p_proc_ready->sleep_ticks = mill_seconds * HZ / 1000;
    schedule();
}

PUBLIC void sys_disp_str(char* str)
{
    if (disp_pos >= 80 * 25 * 2) {
         disp_pos = 0;
        for (int i = 0; i < 80 * 25 * 2; ++i)
        {
            disp_str(" ");
        }
        disp_pos = 0;
    }
    int colors[] = {1, 2, 3, 4, 5, 6};
    disp_color_str(str, colors[(p_proc_ready - proc_table)%5]);
    disp_str("");
}

PUBLIC void sys_sem_p(SEMAPHORE* sem)
{
    sem->value--;
    if (sem->value < 0) {
        sem->queue[sem->wp] = p_proc_ready;
        sem->wp = (sem->wp+1) % 20;
        p_proc_ready->blocked = 1;
        schedule();
    }
}

PUBLIC void sys_sem_v(SEMAPHORE* sem)
{
    sem->value++;
    if (sem->value <= 0) {
        sem->queue[sem->rp]->blocked = 0;
        sem->rp = (sem->rp+1) % 20;
        schedule();
    }
}
