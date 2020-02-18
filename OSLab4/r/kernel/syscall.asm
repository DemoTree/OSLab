
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%include "sconst.inc"

_NR_get_ticks       equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
_NR_sleep			equ 1
_NR_my_disp			equ 2
_NR_P				equ 3
_NR_V				equ 4
_NR_my_disp_int		equ 5
INT_VECTOR_SYS_CALL equ 0x90

; 导出符号
global	get_ticks
global  sleep
global	my_disp
global 	P
global  V
global  my_disp_int

bits 32
[section .text]

; ====================================================================
;                              get_ticks
; ====================================================================
get_ticks:
	mov	eax, _NR_get_ticks
	int	INT_VECTOR_SYS_CALL
	ret

sleep:
	mov	eax, _NR_sleep
	mov ebx, [esp + 4]
	int	INT_VECTOR_SYS_CALL
	ret

my_disp:
	mov	eax, _NR_my_disp
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	int	INT_VECTOR_SYS_CALL
	ret

P:
	mov	eax, _NR_P
	mov ebx, [esp + 4]
	int	INT_VECTOR_SYS_CALL
	ret

V:
	mov	eax, _NR_V
	mov ebx, [esp + 4]
	int	INT_VECTOR_SYS_CALL
	ret

my_disp_int:
	mov	eax, _NR_my_disp_int
	mov ebx, [esp + 4]
	int	INT_VECTOR_SYS_CALL
	ret
