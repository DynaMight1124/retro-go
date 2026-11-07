; Tiny386 image-builder MBR, 2026. SPDX-License-Identifier: MIT
; Single active partition, INT 13h extensions (SeaBIOS), preserve boot drive.
bits 16
org 0x600
start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    cld
    mov si, 0x7c00
    mov di, 0x600
    mov cx, 256
    rep movsw
    jmp 0:relocated
relocated:
    sti
    mov [drive], dl
    mov eax, [0x600+446+8]
    mov [packet+8], eax
    mov si, packet
    mov ah, 0x42
    int 0x13
    jc error
    cmp word [0x7dfe], 0xaa55
    jne error
    mov dl, [drive]
    mov si, 0x600+446
    jmp 0:0x7c00
error:
    mov si, message
.print:
    lodsb
    test al, al
    jz .halt
    mov ah, 0x0e
    mov bx, 7
    int 0x10
    jmp .print
.halt:
    sti
    hlt
    jmp .halt
drive: db 0
message: db 'Tiny386: disk boot failed',0
align 4
packet: db 0x10,0
    dw 1,0x7c00,0
    dq 0
times 446-($-$$) db 0
times 64 db 0
dw 0xaa55
