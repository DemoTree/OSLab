    org 07c00h
    mov ax, cs
    mov ds, ax
    mov es, ax
    call HelloWorld
    jmp $

HelloWorld:
    mov ax, msg
    mov bp, ax
    mov cx, 9
    mov ax, 01301h
    mov bx, 000ch
    mov dl, 0
    int 10h
    ret

msg: db "Hello OS"
times 510-($-$$) db 0
dw 0xaa55