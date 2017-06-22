data segment
	val1 db 'string'
	valb db 011001b
	val2 dd 1518E0Dh
	val3 equ 0
data ends

assume cs:code, ds:data
code segment
	val4 dd 5
main:
	jbe  main2
	aaa
	inc  edx
	div  gs:val2[edx + esi]
	add  al, ah
	cmp  ecx, dword ptr gs:val1[esi + edi]
	and  ds:val1[ecx + edi], al
	imul eax, 10011011b
	div  val4
	if val3
		or   val2[edx + ebx], 127
	else
		or   val2[edx + ebx], 0FFFFFh
	endif
main2:
	jbe  main
code ends 
end main