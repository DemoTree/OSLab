section .data
message db "please input x and y: ", 0Ah
.len equ $-message
enter_break db 0Ah

section .bss
len: resd 1 ;输出数长度
x_len: resd 1 ;第一个数长度
y_len resd 1 ;第二个数长度
sinput resd 41 ;输入内容
carry resd 1 ;进位，0或1,乘法加法都用
result resd 41 ;乘法结果
res_len resd 1 ;结果长度
index resd 1 ;乘法当前值
temp resd 1 ;放入结果时暂用值

section .text
global _start

_start:
    ;输入提示符
    mov edx, message.len
    mov ecx, message
    mov ebx, 1
    mov eax, 4
    int 80h
	
    ;初始化
    mov dword[len], 0
    mov dword[x_len], 0
    mov dword[y_len], 0
    mov dword[sinput], 0
    mov dword[carry], 0
    mov esi, 0
    mov edi, 0

    ;读入输入
    mov eax, 3
    mov ebx, 0
    mov edx, 42
    mov ecx, sinput
    int 80h

;计算第一个数长度
x_input:
    cmp byte[sinput+esi], 20h
    je x_calc

    inc esi
    jmp x_input

x_calc:
    mov dword[x_len],esi
    ;mov eax, dword[x_len]
    ;call iprintLF

;计算第二个数长度
y_input:
    cmp byte[sinput+esi], 0Ah
    je y_calc
	
    inc esi
    jmp y_input
	
y_calc:	
    sub esi, dword[x_len]
    mov eax, esi
    dec eax
    mov dword[y_len], eax
    ;call iprintLF

    mov esi, dword[x_len]
    mov edi, dword[y_len]

;末尾相加
addition:
    xor al, al
    mov al, byte[sinput+esi-1]
    sub al, 30h
    ;call iprint

    xor bl, bl
    push esi
    mov esi, dword[x_len]
    mov bl, byte[sinput+esi+edi]
    pop esi
    sub bl, 30h
    ;push eax
    ;mov eax, ebx
    ;call iprint
    ;pop eax

    xor edx, edx
    push ebx
    push eax
    add eax, ebx
    add eax, dword[carry]
    mov ebx, 10
    div ebx
    ;call iprintLF   
    mov dword[carry], 0
    ;有进位压入栈
    cmp eax, 0 
    je continue
    mov dword[carry], 1

continue:
    ;mov eax, edx
    ;call iprintLF
    pop eax
    pop ebx
    ;当前位压入栈
    push edx
    ;push eax
    ;mov eax, edx
    ;call iprintLF
    ;pop eax

    ;push eax
    ;mov eax, esi
    ;call iprint
    ;mov eax, edi
    ;call iprintLF
    ;pop eax

    dec esi
    dec edi

    inc dword[len]
    cmp esi, 0
    je y_addition_buffer
    
    cmp edi, 0
    je x_addition_buffer
    jmp addition

x_addition_buffer:
    cmp esi,0
    je output_buffer

x_addition:
    xor al, al
    mov al, byte[sinput+esi-1]
    sub al, 30h

    xor edx, edx
    add eax, dword[carry]
    mov ebx, 10
    div ebx
    ;初始化
    mov dword[carry], 0
    ;有进位
    cmp eax, 0 
    je x_continue
    mov dword[carry], 1

x_continue:
    ;当前位压入栈
    push edx
    ;push eax
    ;mov eax, edx
    ;call iprintLF
    ;pop eax

    ;push eax
    ;mov eax, esi
    ;call iprintLF
    ;pop eax

    inc dword[len]
    dec esi
    cmp esi, 0
    je output_buffer
    
    jmp x_addition  

y_addition_buffer:
    mov esi, dword[y_len]
    cmp edi,0
    je output_buffer
    
y_addition:
    xor bl, bl
    mov bl, byte[sinput+esi]
    sub bl, 30h

    xor edx, edx
    mov eax, ebx
    add eax, dword[carry]
    mov ebx, 10
    div ebx

    mov dword[carry], 0
    ;有进位
    cmp eax, 0
    je y_continue
    mov dword[carry], 1

y_continue:
    ;当前位压入栈
    push edx
    ;push eax
    ;mov eax, edx
    ;call iprintLF
    ;pop eax

    inc dword[len]
    dec edi
    dec esi
    
    cmp edi, 0
    je output_buffer
   
    jmp y_addition

output_buffer:

    cmp dword[carry], 0
    jz output
    
    mov eax, dword[carry]
    call iprint
        
output:
    dec dword[len]

    pop eax
    call iprint

    cmp dword[len], 0
    jz part1
    jmp output

;--------------multiplication----------
part1:
    ;打印回车
    mov eax, 4
    mov ebx, 1
    mov ecx, enter_break
    mov edx, 1
    int 80h 
   
    mov dword[carry], 0 
    mov esi, 40

part2:
    ;初始化result为0
    mov byte[result+esi], 0
    cmp esi, 0
    jz part3

    dec esi
    jmp part2

part3:
    mov esi, dword[x_len]
    mov edi, dword[y_len]

    cmp esi,1
    jz x_zero

    cmp edi, 1
    jz y_zero

    jmp part4

x_zero:
    mov al,byte[sinput]
    sub al,30h
    cmp al, 0
    jz zero_print

    jmp part4

y_zero:
    mov al,byte[sinput+esi+1]
    sub al,30h
    cmp al,0
    jz zero_print

    jmp part4

zero_print:
    mov eax, 0
    call iprintLF   

    call quit

part4:
    ;结果从后往前输入
    mov dword[res_len], 0
    jmp multiplication

esi_dec_buffer:
    ;处理最后一个进位
    cmp dword[carry], 0
    jz esi_dec

    mov al, byte[carry]
    sub ebx, 1
    mov byte[result+ebx], al

    ;检查进位
    ;mov al, byte[result+ebx]
    ;call iprintLF

esi_dec:
    dec esi
    cmp esi, 0
    jz output_m_buffer

    mov edi, dword[y_len]

    cmp dword[carry], 0
    jz multiplication
    mov dword[carry], 0


multiplication:
    xor eax, eax
    mov al, byte[sinput+esi-1]
    sub al, 30h
    ;call iprint

    xor ebx, ebx
    push esi
    mov esi, dword[x_len]
    mov bl, byte[sinput+esi+edi]
    sub bl, 30h
    pop esi
    ;push eax
    ;mov eax, ebx
    ;call iprintLF
    ;pop eax

    mul ebx
    ;call iprintLF

    ;和进位相加
    add eax, dword[carry]
    ;call iprintLF
    ;和结果当前位相加
    push eax
    push ebx
    mov eax, dword[x_len]
    mov ebx, dword[y_len]
    add eax, ebx
    sub eax, esi
    sub eax, edi
    xor ecx, ecx
    mov ebx, 40
    sub ebx, eax
    mov cl, byte[result+ebx]
    pop ebx
    pop eax

    add al, cl
    ;判断进位
    xor edx, edx
    mov ebx, 10
    div ebx
    ;有进位
    mov dword[carry],0
    cmp eax, 0 
    je continue_m
    mov dword[carry], eax

continue_m:

    ;push eax
    ;mov eax, edx
    ;call iprint
    ;mov eax, dword[carry]
    ;call iprintLF
    ;pop eax  

    ;余数放入结果
    mov eax, dword[x_len]
    mov ebx, dword[y_len]
    add eax, ebx
    sub eax, esi
    sub eax, edi
    mov ebx, 40
    sub ebx, eax
    mov byte[result+ebx], dl
    mov eax, 41
    sub eax, ebx
    mov dword[res_len], eax

    xor eax, eax
    mov al, byte[result+ebx]
    ;call iprintLF

    dec edi

    cmp edi, 0
    je esi_dec_buffer

    jmp multiplication


output_m_buffer:
    cmp dword[carry],0
    jz output_m

    xor eax, eax
    mov al, byte[carry]
    mov byte[result+ebx], al
    inc dword[res_len]

output_m:
    mov ebx, dword[res_len]
    cmp ebx, 0
    je quit_buffer
    
    xor eax, eax
    xor ecx, ecx
    mov ecx, 41
    sub ecx, ebx
    mov al, byte[result+ecx]
    call iprint

    dec dword[res_len]
    jmp output_m

quit_buffer:
    mov eax, 4
    mov ebx, 1
    mov ecx, enter_break
    mov edx, 1
    int 80h 

    call quit

    


;-------------functions.asm----------------
;------------------------------------------
; void iprint(Integer number)
; Integer printing function (itoa)
iprint:
    push    eax             ; preserve eax on the stack to be restored after function runs
    push    ecx             ; preserve ecx on the stack to be restored after function runs
    push    edx             ; preserve edx on the stack to be restored after function runs
    push    esi             ; preserve esi on the stack to be restored after function runs
    mov     ecx, 0          ; counter of how many bytes we need to print in the end
 
divideLoop:
    inc     ecx             ; count each byte to print - number of characters
    mov     edx, 0          ; empty edx
    mov     esi, 10         ; mov 10 into esi
    idiv    esi             ; divide eax by esi
    add     edx, 48         ; convert edx to it's ascii representation - edx holds the remainder after a divide instruction
    push    edx             ; push edx (string representation of an intger) onto the stack
    cmp     eax, 0          ; can the integer be divided anymore?
    jnz     divideLoop      ; jump if not zero to the label divideLoop
 
printLoop:
    dec     ecx             ; count down each byte that we put on the stack
    mov     eax, esp        ; mov the stack pointer into eax for printing
    call    sprint          ; call our string print function
    pop     eax             ; remove last character from the stack to move esp forward
    cmp     ecx, 0          ; have we printed all bytes we pushed onto the stack?
    jnz     printLoop       ; jump is not zero to the label printLoop
 
    pop     esi             ; restore esi from the value we pushed onto the stack at the start
    pop     edx             ; restore edx from the value we pushed onto the stack at the start
    pop     ecx             ; restore ecx from the value we pushed onto the stack at the start
    pop     eax             ; restore eax from the value we pushed onto the stack at the start
    ret
 
 
;------------------------------------------
; void iprintLF(Integer number)
; Integer printing function with linefeed (itoa)
iprintLF:
    call    iprint          ; call our integer printing function
 
    push    eax             ; push eax onto the stack to preserve it while we use the eax register in this function
    mov     eax, 0Ah        ; move 0Ah into eax - 0Ah is the ascii character for a linefeed
    push    eax             ; push the linefeed onto the stack so we can get the address
    mov     eax, esp        ; move the address of the current stack pointer into eax for sprint
    call    sprint          ; call our sprint function
    pop     eax             ; remove our linefeed character from the stack
    pop     eax             ; restore the original value of eax before our function was called
    ret
 
 
;------------------------------------------
; int slen(String message)
; String length calculation function
slen:
    push    ebx
    mov     ebx, eax
 
nextchar:
    cmp     byte [eax], 0
    jz      finished
    inc     eax
    jmp     nextchar
 
finished:
    sub     eax, ebx
    pop     ebx
    ret
 
 
;------------------------------------------
; void sprint(String message)
; String printing function
sprint:
    push    edx
    push    ecx
    push    ebx
    push    eax
    call    slen
 
    mov     edx, eax
    pop     eax
 
    mov     ecx, eax
    mov     ebx, 1
    mov     eax, 4
    int     80h
 
    pop     ebx
    pop     ecx
    pop     edx
    ret
 
 
;------------------------------------------
; void sprintLF(String message)
; String printing with line feed function
sprintLF:
    call    sprint
 
    push    eax
    mov     eax, 0AH
    push    eax
    mov     eax, esp
    call    sprint
    pop     eax
    pop     eax
    ret
 
 
;------------------------------------------
; void exit()
; Exit program and restore resources
quit:
    mov     ebx, 0
    mov     eax, 1
    int     80h
    ret
