[bits 16]
[org 0x7c00]

start:
    mov bx, 0
    mov ah, 0xe
    mov al, 'H'
    int 10h
    mov al, 'e'
    int 10h
    mov al, 'l'
    int 10h
    mov al, 'l'
    int 10h
    mov al, 'o'
    int 10h
    mov al, ' '
    int 10h
    mov al, 'W'
    int 10h
    mov al, 'o'
    int 10h
    mov al, 'r'
    int 10h
    mov al, 'l'
    int 10h
    mov al, 'd'
    int 10h
    mov al, '!'
    int 10h
    hlt

times 510 - ($ - $$) db 0
dw 0xaa55
