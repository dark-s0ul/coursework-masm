data segment
	valb db  5
	s1   db  "Igihi"
	valw dw  6899
	vald dd  5433ach
	ae   equ [bp + 10]
data ends

code1 segment
assume cs:code1, ds:data
label1:
	cld 
	pop	ax
	mov	al, cl
	mul	byte ptr ss:[esp + 80h]
	xor	cx, [eax + 5]
	sub	[si + 2], eax
	sub	ae, ebx
	cmp	al, 03h
	jc	label2
	jmp	[esp + 5]
	add	dword ptr [bp + 10], 01001b
label2:
	add	word ptr[si + 3], 1
code1 ends
end