; SPDX-License-Identifier: MIT
; Cap free XMS for the following game by reserving excess blocks until reboot.
; NASM -f bin xmscap.asm -o XMSCAP.COM
; No interrupt hooks, disk access, paging or API spoofing. Accept 0..1024 KiB.
bits 16
org 100h
jmp start
entry: dd 0
limit: dw 0
handles: dw 0
require_ems: db 0
allocated: times 64 dw 0
failed: db 'XMS limit could not be applied. Reboot or change memory settings.',13,10,'$'
start:
cli
mov ax,cs
mov ss,ax
mov sp,stacktop
sti
push cs
pop ds
push cs
pop es
mov bx,(programend-$$+100h+15)/16
mov ah,4ah
int 21h
jc fail
xor ax,ax
mov si,81h
xor cx,cx
mov cl,[80h]
skip:
test cx,cx
jz fail
lodsb
dec cx
cmp al,' '
je skip
dec si
inc cx
xor bx,bx
parse:
jcxz parsed
lodsb
dec cx
cmp al,' '
je trailing
sub al,'0'
cmp al,9
ja fail
xor ah,ah
imul bx,bx,10
add bx,ax
cmp bx,1024
ja fail
jmp parse
trailing:
jcxz parsed
lodsb
dec cx
cmp al,' '
je trailing
cmp al,'E'
jne fail
cmp cx,2
jb fail
lodsw
sub cx,2
cmp ax,'MS'
jne fail
mov byte [require_ems],1
spaces:
jcxz parsed
lodsb
dec cx
cmp al,' '
jne fail
jmp spaces
parsed:
mov [limit],bx
cmp byte [require_ems],0
je xms
mov ah,40h
int 67h
test ah,ah
jnz fail
mov ah,41h
int 67h
test ah,ah
jnz fail
test bx,bx
jz fail
mov ah,42h
int 67h
test ah,ah
jnz fail
test bx,bx
jz fail
xms:
mov ax,4300h
int 2fh
cmp al,80h
jne fail
mov ax,4310h
int 2fh
mov [entry],bx
mov [entry+2],es
reserve:
mov ah,8
call far [entry]
cmp dx,[limit]
jbe done
test ax,ax
jz fail
sub dx,[limit]
cmp dx,ax
jbe allocate
mov dx,ax
allocate:
mov ah,9
call far [entry]
test ax,ax
jz fail
mov bx,[handles]
shl bx,1
mov [allocated+bx],dx
inc word [handles]
cmp word [handles],64
jb reserve
jmp fail
done:
cmp word [handles],0
je exit
; Release the inherited environment. Only our XMS handles need to survive.
mov es,[2ch]
mov ah,49h
int 21h
mov dx,(programend-$$+100h+15)/16
mov ax,3100h
int 21h
exit:
mov ax,4c00h
int 21h
fail:
cmp word [handles],0
je message
dec word [handles]
mov bx,[handles]
shl bx,1
mov dx,[allocated+bx]
mov ah,0ah
call far [entry]
jmp fail
message:
mov dx,failed
mov ah,9
int 21h
mov ax,4c01h
int 21h
; Keep the PSP and handle ownership alive; no code is called after residency.
resident_end:
times 512 db 0
stacktop:
programend:
