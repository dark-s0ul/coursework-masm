Data segment
   Zminna   db  10011101b
   string   db  'Gys'
   kd       dd  223a34h
   flag     equ 1
Data ends

assume cs:code
Code segment
main:
   pusha
   jb    sdfds
   inc   ebx
   dec   dword ptr [ecx + esi + 6]
   xchg  ecx, ebx
   lea   ebx, [eax + ebx + 1]
   and   [eax + ecx + 4], ecx
   mov   ebx, 9
   if flag
      or    byte ptr [eax + esi + 5], 4
   else
      or    byte ptr [eax + esi + 5], 7
   endif
sdfds:
   jb main
Code ends
end 