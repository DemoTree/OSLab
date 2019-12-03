
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
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

#define CHAIR_NUM 2

int waiting;
int customer_id;
int sleep = 1;
SEMAPHORE customer, barber, mutex;

void barber_process();
void customer_process();
void my_dispInt(int x){
    char str[5] = {0};
    for (int i = 3; i >= 0; --i)
    {
        str[i] = (x%10) + '0';
        x = x /10;
    }
    for (int i = 0; i < 3; ++i)
    {
        if(str[i]=='0')
            str[i] = ' ';
        else 
            break;
    }
    my_disp_str(str);
}

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16		selector_ldt	= SELECTOR_LDT_FIRST;
	int i;
	for (i = 0; i < NR_TASKS; i++) {
		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;			// pid

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)
			| RPL_TASK;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

        waiting = 0;
        customer_id = 0;
        customer.value = barber.value = 0;
        mutex.value = 1;
        customer.rp = barber.rp = mutex.rp = 0;
        customer.wp = barber.wp = mutex.wp = 0;

    

	k_reenter = 0;
	ticks = 0;

	p_proc_ready	= proc_table;
    proc_table[0].ticks = proc_table[0].priority =  10;
    proc_table[1].ticks = proc_table[1].priority =  20;
    proc_table[2].ticks = proc_table[2].priority =  16;
    proc_table[3].ticks = proc_table[3].priority =  17;
    proc_table[4].ticks = proc_table[4].priority =  18;

    proc_table[0].sleep_ticks = proc_table[0].blocked = 0;
    proc_table[1].sleep_ticks = proc_table[1].blocked = 0;
    proc_table[2].sleep_ticks = proc_table[2].blocked = 0;
    proc_table[3].sleep_ticks = proc_table[3].blocked = 0;
    proc_table[4].sleep_ticks = proc_table[4].blocked = 0;

        /* 初始化 8253 PIT */
        out_byte(TIMER_MODE, RATE_GENERATOR);
        out_byte(TIMER0, (u8) (TIMER_FREQ/HZ) );
        out_byte(TIMER0, (u8) ((TIMER_FREQ/HZ) >> 8));

        put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
        enable_irq(CLOCK_IRQ);                     /* 让8259A可以接收时钟中断 */
   disp_pos = 0;
   for (int i = 0; i < 80 * 25 * 2; ++i)
       {
           disp_str(" ");
       }
        disp_pos = 0;
	restart();

        while(1){}
}

void barber_process(){
        my_disp_str("I, Barber, am sleeping\n");
    while(1){
        sem_p(&customer);
        if(sleep > 0){
            sleep = 0;
            my_disp_str("Barber wakes up\n");
        }
        my_disp_str("Barber cuts hair...\n");
        process_sleep(40000);/* hari is being cut*/
        my_disp_str("Barber'work done!\n");
        sem_v(&barber);
    }
}
void customer_process(){
     while (1) {
        sem_p(&mutex);
        customer_id++;
        int id = customer_id;
        my_disp_str("customer ");
        my_dispInt(id);
        if (waiting >= CHAIR_NUM) {
            my_disp_str(" leave, there is no chair left\n");
            sem_v(&mutex);
        } else {
            waiting++;
            my_disp_str(" sit down, and wait\n");
            sem_v(&mutex);
            sem_v(&customer);
            sem_p(&barber);
            my_disp_str("customer ");
            my_dispInt(id);
            my_disp_str(" got haircut, leave\n");
            sem_p(&mutex);
            waiting--;
            sem_v(&mutex);
        }
        process_sleep(10000);
    }
}
/*======================================================================*
                               TestA
 *======================================================================*/
void A()
{
        while (1) {            
        }
}

void B()
{
    barber_process();
}

void C()
{
   customer_process();
}

void D()
{
     customer_process();
}
void E()
{
    customer_process();
}