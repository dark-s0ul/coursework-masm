data segment
	val1 db  'sdfsdf'
	val2 dw  1F7Ch
	val3 dd  7c7bah
	val4 equ 0h
data ends

code segment
	val5 equ 9h
	val6 equ val3[eax * 8h]
main:
	jz    lend
	stosd
	dec   ebx
	inc   val1[esp * 2h]
	xor   ecx, ebx
	or    edx, val6
	if val4
		and   val1[ecx * 5h], al
	endif
	mov   dl, val5
lend:
	jz    main
code ends
end
