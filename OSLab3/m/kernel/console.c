
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/


#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE* p_con);

/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY* p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1;	/* 显存总大小 (in WORD) */

	int con_v_mem_size                   = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr      = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit        = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;

	if (nr_tty == 0) {
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else {
		out_char(p_tty->p_console, nr_tty + '0');
		out_char(p_tty->p_console, '#');
	}

	set_cursor(p_tty->p_console->cursor);
}


/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE* p_con)
{
	return (p_con == &console_table[nr_current_console]);
}


/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE* p_con, char ch)
{
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);

	switch(ch) {
	case '\n':
		if (p_con->cursor < p_con->original_addr +
		    p_con->v_mem_limit - SCREEN_WIDTH) {
			//my_add 加上换行标识
			*p_vmem++ = '\n';
			*p_vmem++ = 0x00;
			//end
			p_con->cursor = p_con->original_addr + SCREEN_WIDTH * 
				((p_con->cursor - p_con->original_addr) /
				 SCREEN_WIDTH + 1);
		}
		
		break;
	case '\b':
		//my_add 实现TAB整体删除，回车删除
		if (p_con->cursor > p_con->original_addr) {
			//回车删除
			if (p_con->cursor % SCREEN_WIDTH == 0) {
				p_vmem -= SCREEN_WIDTH * 2;
				for (int i=0; i < SCREEN_WIDTH; i++) {
					if (*p_vmem == '\n') {
						p_con->cursor -= SCREEN_WIDTH - i;
						*p_vmem = ' ';
						*(p_vmem + 1) = DEFAULT_CHAR_COLOR;
						goto end;
					}
					p_vmem += 2;
				}
			}
			//TAB整体删除
			if (*(p_vmem-2) == 0) {
				for (int i=0; i < 4; i++) {
					p_vmem -= 2;
					p_con->cursor--;
					if (*(p_vmem - 2) != 0) break;
				}
			}
			//正常删除
			else {
				p_con->cursor--;
				*(p_vmem-2) = '\0';
				*(p_vmem-1) = DEFAULT_CHAR_COLOR;
			}
		}
		//end
		end:
			break;
	
	//my_add 实现TAB输入
	case '\t':
	{	
		int len = SCREEN_WIDTH - p_con->cursor % SCREEN_WIDTH;
		if(len >= 4){
			for (int i = 0; i < 4; i++){
				*p_vmem++ = '\0';
				*p_vmem++ = DEFAULT_CHAR_COLOR; 
				p_con->cursor++;
			}
		}else{//距右边少于4格从下一行段首开始
			for (; len > 0; len--){
				*p_vmem++ = '\0';
				*p_vmem++ = DEFAULT_CHAR_COLOR; 
				p_con->cursor++;
			}
		}
		
	}
		break;
	//end

	default:
		if (p_con->cursor <
		    p_con->original_addr + p_con->v_mem_limit - 1) {
			*p_vmem++ = ch;
			if(p_con->is_search == 1)
				*p_vmem++ = SEARCH_CHAR_COLOR;
			else
				*p_vmem++ = DEFAULT_CHAR_COLOR;
			p_con->cursor++;
		}
		break;
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}

/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE* p_con)
{
        set_cursor(p_con->cursor);
        set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}



/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console)	/* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
		return;
	}

	nr_current_console = nr_console;

	set_cursor(console_table[nr_console].cursor);
	set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE* p_con, int direction)
{
	if (direction == SCR_UP) {
		if (p_con->current_start_addr > p_con->original_addr) {
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN) {
		if (p_con->current_start_addr + SCREEN_SIZE <
		    p_con->original_addr + p_con->v_mem_limit) {
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else{
	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}

/*======================================================================*
			my_add   refresh_screen 刷新屏幕
 *======================================================================*/

PUBLIC void refresh_screen(CONSOLE* p_con){
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
	//flush(p_con);
	while (p_con->cursor > p_con -> original_addr) {
		p_con->cursor--;
		*(p_vmem-2) = '\0';
		*(p_vmem-1) = DEFAULT_CHAR_COLOR;
		p_vmem -= 2;
		//flush(p_con);
	}
	p_con->is_search = 0;
	set_cursor(p_con->original_addr);
}

/*======================================================================*
			my_add   search_char 查找
 *======================================================================*/
PUBLIC void search(CONSOLE* p_con, char* p, int length){
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
	u8* p_vmem_base = (u8*)(V_MEM_BASE);
	//遍历输入
	for (int i = 0; i < p_con->cursor - length; ++i){
		if (*(p_vmem_base+2*i)==*p){
			int temp = 1;
			for (int j = 0; j < length; ++j){
				if (*(p_vmem_base+2*(i+j))!=*(p+j)){	
					temp = 0;
					break;
				}
			}
			if (temp == 1){
				for (int j = 0; j < length; ++j){
					*(p_vmem_base+2*(i+j)+1) = SEARCH_CHAR_COLOR;
				}
			}
		}
		
	}
};

PUBLIC void stop_search(TTY* p_tty, int length){
	CONSOLE* p_con = p_tty->p_console;
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
	u8* p_vmem_base = (u8*)(V_MEM_BASE);
	//恢复颜色
	for (int i = 0; i < p_con->cursor; ++i){
		if(*(p_vmem_base+2*i)=='\n'){
			*(p_vmem_base+2*i+1) = 0x00;
		}else{
			*(p_vmem_base+2*i+1) = DEFAULT_CHAR_COLOR;
		}
	}
	//删除搜索内容
	for (int i = 0; i < length; ++i){
		put_key(p_tty, '\b');
	}
};


