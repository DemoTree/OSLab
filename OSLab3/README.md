# README

在Orange's实现源码chapter7 m基础上修改而成

##### 源码已实现：

1. 可以输入并显示a-z，A-Z，0-9
2. shift组合键及大写锁定两种方式的大小写切换
3. 回车键换行
4. 退格键删除输入内容
5. 支持空格键
6. 光标闪烁，随输入字符的位置变化

##### 待实现：

1. 支持TAB键
2. 每隔20s左右清空屏幕
3. Esc查找
4. 回车换行后的删除
5. （附加功能）control+z组合键撤回上一个输入的字符

### 修改内容

1. Makefile：

   添加 run 指令直接运行

   CFLAGS后添加 `-fno-stack-protector` 避免 `__stack_chk_fail` 报错

   CC后添加 `-w` 忽略 gcc 编译 warning 警告

2. console.h

   添加搜索时颜色`SEARCH_CHAR_COLOR`

   结构体s_console中添加 is_search 判断是否是搜索状态

3. proto.h

   添加 put_key(), refresh_screen(), search(), stop_search()方法的引用申明

4. console.c

   添加 search()方法用于查找

   添加 stop_search()方法用于查找结束后恢复原始状态

   修改 out_char()实现让查找关键字显示为蓝色，TAB 的输入和 TAB 、回车的整体删除

   添加refresn_screen()方法用于刷新屏幕

5. tyy.c

   修改 task_tty()实现屏幕初始化

   修改 in_process()实现查找模式的键盘动作识别

   将 put_key()由 PRIVATE 改为 PUBLIC 供 console.c 调用

   将 权限最高的TestA放到此文件中实现屏幕刷新

6. main.c

   将 TestA 注释掉

   

   

   

   

   









2017:

1. console.c
2. tty.c
3. seek.c

2018:

1. console.c 搜索
2. tty.c 清空屏幕

hxh: 7c

1. kernel/tty.c 主要功能
2. kernel/keyboard.c 大小写切换 输入组合键
3. include/proc.h 调整任务数量
4. kernel/global.c 调整任务切换

wrh:

1. console.c tab 查找 

Lab3:

1. concole.c tty.c main.c 
