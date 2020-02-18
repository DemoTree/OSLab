
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

#define MAX 3 //一本书可供读者个数，支持1，2，3
#define TIME 10000 //自定义时间片
//颜色常量
#define A_color 0x0A
#define B_color 0x0B
#define C_color 0x0D
#define D_color 0x0E
#define E_color 0x09
#define F_color 0x0C

SEMAPHORE S;
SEMAPHORE lmutex;//限制同时读者个数
SEMAPHORE rmutex;//读者进程互斥修改 readcount
SEMAPHORE fmutex;//文件资源，读者写者互斥访问
int readcount = 0;//记录读者数
int flag = 0;//记录当前是读还是写，0为读，1为写
/*****写者优先添加******/
int writecount = 0;//记录写者数
SEMAPHORE xmutex;
SEMAPHORE ymutex;
SEMAPHORE zmutex;

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

	//my_add
	proc_table[0].ticks = proc_table[0].priority =  1;
	proc_table[1].ticks = proc_table[1].priority =  1;
	proc_table[2].ticks = proc_table[2].priority =  1;
	proc_table[3].ticks = proc_table[3].priority =  1;
	proc_table[4].ticks = proc_table[4].priority =  1;
	proc_table[5].ticks = proc_table[5].priority =  1;

	proc_table[0].sleep_ticks = proc_table[0].blocked = 0;
    proc_table[1].sleep_ticks = proc_table[1].blocked = 0;
    proc_table[2].sleep_ticks = proc_table[2].blocked = 0;
    proc_table[3].sleep_ticks = proc_table[3].blocked = 0;
    proc_table[4].sleep_ticks = proc_table[4].blocked = 0;
    proc_table[5].sleep_ticks = proc_table[5].blocked = 0;

	readcount = 0;
	flag = -1;
	lmutex.value = MAX;//读者个数
	/*****读者优先用******/	
	rmutex.value = 1;//保护读者
	fmutex.value = 1;//保护文件
	S.value = 1;
	/*****写者优先用******/	
	writecount = 0;
	xmutex.value = 1;
	ymutex.value = 1;
	zmutex.value = 1;

	//清空屏幕
	disp_pos = 0;
	for (int k = 0; k < 80*25; k++) {
		disp_str(" ");
	}
	disp_pos = 0;
	//end

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

//normal普通进程
void TestF()
{
	while(1){
		if(flag == 0){
			my_disp("F:READING (", F_color);
			my_disp_int(readcount);
			my_disp(" people)\n", F_color);
		}else if(flag == 1){
			my_disp("F:WRITING\n", F_color);
		}
		milli_delay(TIME);
	}
}

/*======================================================================*
                               读者优先
 *======================================================================*/

void TestA()//reader1
{	
	while(1){

		P(&lmutex);//控制个数
		
		P(&S);
		P(&rmutex);//读者想读书，申请读者计数器资源
		if(readcount==0){//第一个读，要等书空出来才有读取权限
			P(&fmutex);
		}
		readcount++;//读者个数+1
		V(&rmutex);//释放读者计数器资源
		V(&S);

		flag = 0;

		my_disp("A:reader starts to read.\n", A_color);//开始读书
		
		milli_delay(2*TIME);//读书消耗2个时间片

		my_disp("A:reader has finished.\n", A_color);//读完了
		
		P(&rmutex);//读者想离开，申请读者计数器资源
		readcount--;//读者个数-1
		if(readcount==0){//如果没有读者了，释放书资源，此时才可以写者才可以写
			V(&fmutex);
		}
		V(&rmutex);//读者离开，释放读者计数器资源

		V(&lmutex);
	}
}

void TestB()//reader2
{
	while(1){

		P(&lmutex);

		P(&S);
		P(&rmutex);//申请读者计数器资源
		if(readcount==0){
			P(&fmutex);
		}
		readcount++;
		V(&rmutex);//释放读者计数器资源
		V(&S);

		flag = 0;

		my_disp("B:reader starts to read.\n", B_color);
		
		milli_delay(3*TIME);

		my_disp("B:reader has finished.\n", B_color);
		
		P(&rmutex);
		readcount--;
		if(readcount==0){
			V(&fmutex);
		}
		V(&rmutex);
		
		V(&lmutex);
	}
}

void TestC()//reader3
{
	while(1){
		P(&lmutex);

		P(&S);
		P(&rmutex);//申请读者计数器资源
		if(readcount==0){
			P(&fmutex);
		}
		readcount++;
		V(&rmutex);//释放读者计数器资源
		V(&S);

		flag = 0;

		my_disp("C:reader starts to read.\n", C_color);
		
		milli_delay(3*TIME);
		
		my_disp("C:reader has finished.\n", C_color);
		
		P(&rmutex);
		readcount--;
		if(readcount==0){
			V(&fmutex);
		}
		V(&rmutex);
		
		V(&lmutex);
	}
}

void TestD()//writer1
{
	while(1){
	
		P(&S);
		P(&fmutex);//写者想写，申请文件资源
		flag = 1;
		
		my_disp("D:writer starts to write.\n", D_color);//写者开始写
		
		milli_delay(4*TIME);//D进程写耗费4个时间片

		my_disp("D:writer has finished.\n", D_color);//写者结束写

		V(&fmutex);//写者写完了，释放文件资源
		V(&S);
	}
}

void TestE()//writer2
{
	while(1){
		
		P(&S);
		P(&fmutex);
		flag = 1;

		my_disp("E:writer starts to write.\n", E_color);
		
		milli_delay(4*TIME);

		my_disp("E:writer has finished.\n", E_color);
		
		V(&fmutex);
		V(&S);
	}
}

/*======================================================================*
                               写者优先
 *======================================================================*/
/*
void TestA()//reader1
{
	while(1){
		P(&lmutex);

		P(&zmutex);//避免写者同时与多个读者竞争
			P(&rmutex);//读者想读书，申请读者计数器资源
				P(&xmutex);//防止一次多个读者修改readcount
				readcount++;
				if(readcount==1){//读者是第一个，申请文件资源
					P(&fmutex);
				}
				V(&xmutex);		
			V(&rmutex);//释放读者计数器资源
		V(&zmutex);

		flag = 0;

		my_disp("A:reader starts to read.\n", A_color);//读者开始读书
		
		milli_delay(2*TIME);//消耗2个时间片
		
		my_disp("A:reader  has finished.\n", A_color);//读者结束读书
		
		P(&xmutex);//读者想离开，申请读者计数器资源
		readcount--;
		if(readcount==0){//读者已全部离开，释放文件资源
			V(&fmutex);
		}
		V(&xmutex);//读者离开，释放读者计数器资源

		V(&lmutex);
	}
}

void TestB()//reader2
{
	while(1){
		P(&lmutex);

		P(&zmutex);
			P(&rmutex);//申请读者计数器资源
				P(&xmutex);
				readcount++;
				if(readcount==1){
					P(&fmutex);
				}
				V(&xmutex);		
			V(&rmutex);//释放读者计数器资源
		V(&zmutex);

		flag = 0;

		my_disp("B:reader starts to read\n", B_color);
		
		milli_delay(3*TIME);
		
		my_disp("B:reader has finished.\n", B_color);
		
		P(&xmutex);
		readcount--;
		if(readcount==0){
			V(&fmutex);
		}
		V(&xmutex);

		V(&lmutex);
	}
}

void TestC()//reader3
{
	while(1){
		P(&lmutex);

		P(&zmutex);
			P(&rmutex);//申请读者计数器资源
				P(&xmutex);
				readcount++;
				if(readcount==1){
					P(&fmutex);
				}
				V(&xmutex);		
			V(&rmutex);//释放读者计数器资源
		V(&zmutex);

		flag = 0;

		my_disp("C:reader starts to read.\n", C_color);
		
		milli_delay(3*TIME);
		
		my_disp("C:reader has finished.\n", C_color);
		
		P(&xmutex);
		readcount--;
		if(readcount==0){
			V(&fmutex);
		}
		V(&xmutex);

		V(&lmutex);
	}
}

void TestD()//writer1
{
	while(1){
	
		P(&ymutex);//确保只有一个写者
		writecount++;
		if(writecount==1){//写者想写，阻断读者计数器
			P(&rmutex);
		}
		V(&ymutex);

		P(&fmutex);//写者想写，申请文件资源
		flag = 1;

		my_disp("D:writer starts to write.\n", D_color);//写者开始写

		milli_delay(4*TIME);//消耗4个时间片

		my_disp("D:writer has finished.\n", D_color);

		V(&fmutex);//写者写完了，释放文件资源

		P(&ymutex);//确保只有一个写者
		writecount--;
		if(writecount==0){//写者写完了，释放读者计数器
			V(&rmutex);
		}
		V(&ymutex);
	}
}

void TestE()//writer2
{
	while(1){
	
		P(&ymutex);
		writecount++;
		if(writecount==1){
			P(&rmutex);
		}
		V(&ymutex);

		P(&fmutex);
		flag = 1;

		my_disp("E:writer starts to write.\n", E_color);
	
		milli_delay(4*TIME);

		my_disp("E:writer has finished.\n", E_color);
		
		V(&fmutex);

		P(&ymutex);
		writecount--;
		if(writecount==0){
			V(&rmutex);
		}
		V(&ymutex);
	}
}*/