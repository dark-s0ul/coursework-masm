;.386

Data1 Segment 
	VAR1B  db 'string' 
	VAR1W  dw 11AFh 
	VAR1D  dd 24523 
	MAINi1 dw main
Data1 ends 

Data2 Segment 
	VAR2B  db 11010001b
	VAR2W  dw 59374 
	VAR2D  dd 13ch 
	MAINi2 dw main4
Data2 ends 

Code1 Segment 
Assume cs:Code1, ds:Data1, gs:Data2, fs:Code2
main1:
	sti
	dec eax
	add bx, VAR2W[bp + si]
	add ebx, dword ptr VAR2W[bp + si]
	add bx, VAR2W[ebp + di]
	cmp ebx, ecx
	or  VAR1B[edx + esi], 127
	or  word ptr VAR1B[edx + esi], 7AD4h
	or  VAR1D[bp + esi], 7AD4h
	and cx, 7Fh
	and cx, 80h
	and ecx, 0FF7Fh
	and ecx, 0FF80h
	jmp VAR2W[bp + si]
main3:
main4:
	jle main1
Code1 ends 

Code2 Segment 
Assume cs:Code2,ds:Data1,gs:Data2 
main2:
	sti
	dec al
	dec ax
	dec eax
	add bx, VAR2W[bp + si]
	cmp bx, cx
	cmp ebp, esi
	or  dword ptr VAR1B[edx + esi], 7AD4h
	or  gs:VAR1D[edx + esi], 7AD4h
	or  VAR1D[bp + si], 7AD4h
	and cl, 1001010b
	and ecx, 233
	jmp MAINi1
	jmp MAINi2
Code2 ends 
END 