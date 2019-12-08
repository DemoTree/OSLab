
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

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

#define TTY_FIRST	(tty_table)
#define TTY_END		(tty_table + NR_CONSOLES)

PRIVATE void init_tty(TTY* p_tty);
PRIVATE void tty_do_read(TTY* p_tty);
PRIVATE void tty_do_write(TTY* p_tty);

//my_add
PRIVATE char s_content[100] = {0};	//查找关键字
PRIVATE int s_length = 0;	//查找关键字长度
CONSOLE* p_con;
//end

/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty()
{
	TTY*	p_tty;

	init_keyboard();

	for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
		init_tty(p_tty);
	}
	select_console(0);

	//my_add 初始化
	p_tty->p_console->is_search = 0;
	char s_content[100] = {0};
	s_length = 0;
	int index=0;
	//end

	while (1) {
		for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
			//初始化屏幕
			if(index==0){
				for(int j=0;j<80*25;j++)
					out_char(p_tty->p_console, '\b');
				index=1;
			}
			tty_do_read(p_tty);
			tty_do_write(p_tty);
			p_con = p_tty -> p_console;
			
		}
	}
}
//my_add 定时刷新屏幕
void TestA()
{
	while (1) {
		if(p_con->is_search==0){
			refresh_screen(p_con);
		}
		milli_delay(100000);
	}
}
//end

/*======================================================================*
			   init_tty
 *======================================================================*/
PRIVATE void init_tty(TTY* p_tty)
{
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;

	init_screen(p_tty);
}

/*======================================================================*
				in_process
 *======================================================================*/
PUBLIC void in_process(TTY* p_tty, u32 key)
{
        char output[2] = {'\0', '\0'};
		
		//my_add ESC查找
		if((key & MASK_RAW) == ESC){
			if (p_tty->p_console->is_search == 0) {
                p_tty->p_console->is_search = 1;
            }else{
                p_tty->p_console->is_search = 0;
				//结束搜索，删除搜索内容，还原颜色
                stop_search(p_tty, s_length);
				char s_content[100] = {0};
				s_length = 0;
            }
		}
		//end

		if(p_tty->p_console->is_search!=2){
			if (!(key & FLAG_EXT)) {
				if(p_tty->p_console->is_search == 1){
					s_content[s_length]=key;
            		s_length++;
				}		
				put_key(p_tty, key);
			} else {
				int raw_code = key & MASK_RAW;
				switch(raw_code) {		
					//my_add 修改ENTER查找方法，添加TAB
					case ENTER:
						if(p_tty->p_console->is_search==1){
							//开始查找，禁止输入
							search(p_tty->p_console, s_content, s_length);
							p_tty->p_console->is_search = 2;
						}else{
							//非查找模式，回车换行
							put_key(p_tty, '\n');						
						}
						break;
					
					case TAB:
						if(p_tty->p_console->is_search == 1){
							s_content[s_length]=key;
            				s_length++;
						}
						put_key(p_tty, '\t');
						break;
					//end
						
					case BACKSPACE:
						put_key(p_tty, '\b');
						if(p_tty->p_console->is_search==1){
							if(s_length>0) s_length--;
						}
						break;
					
					case UP:
						if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
							scroll_screen(p_tty->p_console, SCR_DN);
						}
						break;
					case DOWN:
						if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
							scroll_screen(p_tty->p_console, SCR_UP);
						}
						break;
					case F1:
					case F2:
					case F3:
					case F4:
					case F5:
					case F6:
					case F7:
					case F8:
					case F9:
					case F10:
					case F11:
					case F12:
						/* Alt + F1~F12 */
						if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R)) {
							select_console(raw_code - F1);
						}
						break;
					default:
						break;
				}
			}
		}
}

/*======================================================================*
			      put_key
*======================================================================*/
PUBLIC void put_key(TTY* p_tty, u32 key)
{
	if (p_tty->inbuf_count < TTY_IN_BYTES) {
		*(p_tty->p_inbuf_head) = key;
		p_tty->p_inbuf_head++;
		if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_head = p_tty->in_buf;
		}
		p_tty->inbuf_count++;
	}
}


/*======================================================================*
			      tty_do_read
 *======================================================================*/
PRIVATE void tty_do_read(TTY* p_tty)
{
	if (is_current_console(p_tty->p_console)) {
		keyboard_read(p_tty);
	}
}


/*======================================================================*
			      tty_do_write
 *======================================================================*/
PRIVATE void tty_do_write(TTY* p_tty)
{
	if (p_tty->inbuf_count) {
		char ch = *(p_tty->p_inbuf_tail);
		p_tty->p_inbuf_tail++;
		if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count--;

		out_char(p_tty->p_console, ch);
	}
}


