data segment
	val1 db 'toxa'
	val3 dd 9F6ACDE8h
data ends

code segment
main:
	jnz label1
	das
	idiv eax
	inc  word ptr val2[ecx + eax * 8]
	and  bl, bh
	xor  al, val1[edx + esi * 2]
	cmp  dword ptr val3[edx + esi * 4], ebx
	mov  dl, 255
	adc  word ptr val2[ecx + eax * 2], 0AFh
	jnz  main
label1:
code ends
end
