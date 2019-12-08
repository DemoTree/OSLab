
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

//在等待的人的id
int chairid[CHAIRS];
int chair_in;
int chair_out;


PUBLIC void clearScreen();
PRIVATE void cuthair(int);
PRIVATE void print_num(int,int);
PRIVATE void get_haircut(int);
PRIVATE void come(int,int);
PRIVATE void leave(int);
PRIVATE void finish(int);

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

	proc_table[0].ticks = proc_table[0].blocked = 0;
	proc_table[1].ticks = proc_table[0].blocked = 0;
	proc_table[2].ticks = proc_table[0].blocked = 0;
	proc_table[3].ticks = proc_table[0].blocked = 0;
	proc_table[4].ticks = proc_table[0].blocked = 0;
	

	disp_pos = 0;
	for(i=0;i<80*25;i++){
		disp_str(" ");	
	}
	disp_pos = 0;
	
	k_reenter = 0;
	ticks = 0;
	
	cid = 0;	

	waiting = 0;
	customers.value = 0;
	customers.in = 0;
	customers.out = 0;
	mutex.value = 1;
	mutex.in = 0;
	mutex.out = 0;
	barbers.value = 0;
	barbers.in = 0;
	barbers.out = 0;
	
	chair_in = 0;
	chair_out = 0;

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

/*======================================================================*
                               Normal
 *======================================================================*/
void Normal()
{
	//clearScreen();
	while (1) {
		
		//clearScreen();
		milli_delay(100);
	}
}

/*======================================================================*
                               Barber
 *======================================================================*/
void Barber()
{
	
	while(1){
		//判断理发师是否睡眠
		if(waiting == 0)
			color_print("Barber is now sleeping\n",BARBER_COLOR);
		sem_p(&customers,1);
		sem_p(&mutex,1);
		waiting--;
		sem_v(&barbers);
		sem_v(&mutex);
		
		//sem_p(&printMutex,1);
		//拿出等待队列
		int id = chairid[chair_out];
		int id2 = chairid[chair_out];
		chair_out = (chair_out+1)%CHAIRS;
		cuthair(id);
		//sem_v(&printMutex);
		//理发等待时间
		sleep(HAIRCUT_TIME);
		finish(id2);
	}
}

/*======================================================================*
                               Customer1
 *======================================================================*/
void Customer1()
{
	while(1){
		sem_p(&mutex,2);
		int id = cid;
		int id2 = cid;
		
		cid++;
		//sem_p(&printMutex,2);
		come(id,waiting);
		//sem_v(&printMutex);
		
		if(waiting<CHAIRS){
			waiting++;
			//加入等待队列
			chairid[chair_in] = id2;
			chair_in = (chair_in+1)%CHAIRS;
			
			sem_v(&customers);
			sem_v(&mutex);
			sem_p(&barbers,2);
			
			//sem_p(&printMutex,2);
			//get_haircut(id);
			//sem_v(&printMutex);
		}
		else{
			//sem_p(&printMutex,2);
			leave(id2);
			//sem_v(&printMutex);
			sem_v(&mutex);
		}
		sleep(COSTUMER1_GAP);
	}
}

/*======================================================================*
                               Customer2
 *======================================================================*/
void Customer2()
{
	while(1){
		sem_p(&mutex,3);
		int id = cid;
		int id2 = cid;
		
		cid++;
		//sem_p(&printMutex,3);
		come(id,waiting);
		//sem_v(&printMutex);
		
		if(waiting<CHAIRS){
			waiting++;
			//加入等待队列
			chairid[chair_in] = id2;
			chair_in = (chair_in+1)%CHAIRS;
			sem_v(&customers);
			sem_v(&mutex);
			sem_p(&barbers,3);
			
			//sem_p(&printMutex,3);
			//get_haircut(id);
			//sem_v(&printMutex);
		}
		else{
			leave(id2);
			sem_v(&mutex);
		}
		sleep(COSTUMER2_GAP);
	}
}

/*======================================================================*
                               Customer3
 *======================================================================*/
void Customer3()
{
	while(1){
		sem_p(&mutex,4);
		int id = cid;
		int id2 = cid;
		
		cid++;
		//sem_p(&printMutex,4);
		come(id,waiting);
		//sem_v(&printMutex);
		
		if(waiting<CHAIRS){
			waiting++;
			//加入等待队列
			chairid[chair_in] = id2;
			chair_in = (chair_in+1)%CHAIRS;
			sem_v(&customers);
			sem_v(&mutex);
			sem_p(&barbers,4);
			
			//sem_p(&printMutex,4);
			//get_haircut(id);
			//sem_v(&printMutex);
		}
		else{
			leave(id2);
			sem_v(&mutex);
		}
		sleep(COSTUMER3_GAP);
	}
}

PUBLIC void clearScreen(){
	disp_pos = 0;
	for(int i=0;i<80*25;i++){
		disp_str(" ");
	}
	disp_pos = 0;
}

PRIVATE void cuthair(int cutid){
	char cutting[] = "Barber is cutting ";
	color_print(cutting,BARBER_COLOR);
	print_num(cutid,BARBER_COLOR);
	char customer[] = " customer.\n";
	color_print(customer,BARBER_COLOR);
}
	
PRIVATE void get_haircut(int cutid){
	char customer[] = "Custormer ";
	color_print(customer,CUSTOMER_COLOR);
	print_num(cutid,CUSTOMER_COLOR);
	char cuthair[] = " cut hair.\n";
	color_print(cuthair,CUSTOMER_COLOR);
}

PRIVATE void come(int id,int num){
	char customer[] = "Custormer ";
	color_print(customer,CUSTOMER_COLOR);
	print_num(id,CUSTOMER_COLOR);
	//char output[5];
	//myitoa(output,id);
	//color_print(output,CUSTOMER_COLOR);
	char now[] = " comes.Now ";
	color_print(now,CUSTOMER_COLOR);
	print_num(num,CUSTOMER_COLOR);
	char wait[] = " people are waiting.\n";
	color_print(wait,CUSTOMER_COLOR);
}

PRIVATE void leave(int id){
	char customer[] = "Custormer ";
	color_print(customer,CUSTOMER_COLOR);
	print_num(id,CUSTOMER_COLOR);
	char out[] = " leaves for no chair.\n";
	color_print(out,CUSTOMER_COLOR);
}

PRIVATE void finish(int id){
	char out[] = "Barber finish cutting customer ";
	color_print(out,BARBER_COLOR);
	print_num(id,BARBER_COLOR);
	print("\n");
	//char end[] = ".\n";
	//color_print(end,BARBER_COLOR);
}
	
PRIVATE void print_num(int num,int color){
	char temp[16];
	char res[16];
	for(int i=0;i<16;i++){
		temp[i] = '\0';
		res[i] = '\0';
	}
	int len = 0;
	int q = num;
	while(q!=0){
		temp[len] = q%10+'0';
		len++;
		q = q/10;
	}
	for(int i=0;i<len;i++){
		res[i] = temp[len-1-i];
	}
	if(len==0){
		res[0] = '0';
		res[1] = '\0';
	}else{
		res[len]=='\0';
	}
	color_print(res,color);
}

