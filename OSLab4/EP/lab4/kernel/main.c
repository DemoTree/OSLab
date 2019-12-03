
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
#include <math.h>  
#define INT_MAX 2147483647  
#define INT_MIN (-2147483647-1)//必须是这种表示形式，-2147483648会报错
#define NR_CHAIR 3

int waiting;
int customer_id;
SEMAPHORE customer, barber, mutex;

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

        waiting = 0;  //等待的顾客数目（未理发）
        customer_id = 0;
        customer.value = barber.value = 0;  //customer是等待服务的顾客数目，barber是等待顾客的理发师数目
        mutex.value = 1; //互斥信号量，保护waiting

        memset(0xB8000, 0, 80 * 25 * 2);
        disp_pos = 0;

	k_reenter = 0;
	ticks = 0;

	p_proc_ready	= proc_table;

        /* 初始化 8253 PIT */
        out_byte(TIMER_MODE, RATE_GENERATOR);
        out_byte(TIMER0, (u8) (TIMER_FREQ/HZ) );
        out_byte(TIMER0, (u8) ((TIMER_FREQ/HZ) >> 8));

        put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
        enable_irq(CLOCK_IRQ);                     /* 让8259A可以接收时钟中断 */

	restart();

        while(1){}
}

const char* int_to_str(int iVal)  
{  
    static char str[12];  
    int isNegative = 0;  
    int i = 0,j=0;  
    if (iVal == 0)  
    {  
        str[0] = '0';  
        str[1] = '\0';  
        return str;  
    }  
    if (iVal == INT_MIN)  
    {  
        int_to_str(iVal + 1);  
        char *tmp = str;  
        while (*tmp != '\0')  
            tmp++;  
        tmp--;  
        *tmp += 1;  
        return str;  
    }  
    if (iVal < 0)  
    {  
        iVal *= -1;  
        isNegative = 1;  
        str[i++] = '-';  
        j++;  
    }  
    while (iVal)  
    {  
        str[i++]=iVal % 10+'0';  
        iVal /= 10;  
    }  
    str[i--] = '\0';  
    while (j < i)  
    {  
        char ch = str[i];  
        str[i--] = str[j];  
        str[j++] = ch;  
    }  
    return str;  
  
}

/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
        while (1) {}
}

void B()
{
    process_sleep(10000); //一开始，理发师在睡眠
    new_disp_str("Barber ready to work!\n");
    while(1){
        sem_p(&customer);//查看是否有等候服务的顾客，如果没有，就睡眠

        new_disp_str("Barber begins to cut hair...\n");
        process_sleep(60000);
        new_disp_str("Barber haircut done!\n");

        sem_v(&barber); //理完上个顾客理发师就空闲了，于是释放理发师
    }
}

void CDE()
{
    while (1) {
        sem_p(&mutex);//进入临界区，顾客想坐在椅子上
        customer_id++;

        int my_id = customer_id;
        new_disp_str("Customer ");
        new_disp_str(int_to_str(my_id));
        if (waiting >= NR_CHAIR) {
            new_disp_str(" come, no chair, leave\n");
            sem_v(&mutex); //没有椅子空着，离开，退出临界区
        } else {
            waiting++;  //等候顾客数目+1
            new_disp_str(" come, sit on a chair and wait\n");
            sem_v(&mutex);  //坐在椅子上，退出临界区

            sem_v(&customer); //等候服务的顾客数唤醒理发师
            sem_p(&barber); //如果理发师忙，顾客就等待

            new_disp_str("Customer ");
            new_disp_str(int_to_str(my_id));
            new_disp_str(" got haircut, leave\n");//否则顾客就可以理发

            sem_p(&mutex); //顾客想起身离开，进入临界区
            waiting--; //等待顾客-1
            sem_v(&mutex); //顾客离开了，退出临界区
        }
        process_sleep(30000);
    }
}
