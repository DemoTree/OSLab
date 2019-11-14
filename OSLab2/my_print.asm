global myprint

section .data
color_default:  db  1Bh, '[37;0m', 0
.len            equ $ - color_default
color_red:	db  1Bh, '[31;1m', 0
.len            equ $ - color_red

section .bss

section .text

myprint:
	mov eax, [esp+12];color
	cmp eax, 0
	je print_default
	cmp eax, 1
	je print_red

print_default:
	mov eax, 4
	mov ebx, 1
	mov ecx, color_default
	mov edx, color_default.len
	int 80h
	jmp print_content

print_red:
	mov eax, 4
	mov ebx, 1
	mov ecx, color_red
	mov edx, color_red.len
	int 80h
	jmp print_content

print_content:
	mov eax, 4
	mov ebx, 1
	mov ecx, [esp+4];content
	mov edx, [esp+8];length
	int 80h

;将颜色改回默认
	mov eax, 4
	mov ebx, 1
	mov ecx, color_default
	mov edx, color_default.len
	int 80h
