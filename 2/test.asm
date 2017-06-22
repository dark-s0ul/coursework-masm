Data1 Segment 
	STRING db 'string' 
	VARB   dw 11AFh 
	PFUNC  dw proc1 
Data1 ends 

Data2 Segment 
	VARW   dw LABEL1 
	VARD   dw 13ch 
Data2 ends 

Code1 Segment 
Assume cs:Code1, ds:Data1, gs:Data2
	proc1 PROC FAR
		ret 
	proc1 ENDP 
	not ax 
	mov byte ptr[bx + di + 5h], 10h
	call VARD[bp + di + 6h]
	or   ax, VARB[bx + si + 1h]
	add  ax, bx 
	call proc1
	jge label12
	mov byte ptr[bp + di + 5h], 5h
	mov  gs:VARD[bx + si + 6h], 14h 
label12:
Code1 ends 

Code2 Segment 
Assume cs:Code2,ds:Data1,gs:Data2 
LABEL1:
	mov  VARW[bp + di + 12h],12h 
	call far ptr proc1
	jge  LABEL1 
	call PFUNC[bp + si + 5h]
Code2 ends 
END 