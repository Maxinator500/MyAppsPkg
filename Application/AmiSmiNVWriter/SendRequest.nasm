DEFAULT REL
SECTION .text

global SendRequest

SendRequest:
  mov     ah, 3
  mov     ebx, ecx

  ret
