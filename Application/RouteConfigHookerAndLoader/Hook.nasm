DEFAULT REL

extern ASM_PFX(OriginalRouteConfig)
extern ASM_PFX(FileHandle)
extern ASM_PFX(PrintRouteStatus)
extern ASM_PFX(GlobalVariableGuid)
extern ASM_PFX(Original)
extern ASM_PFX(Returned)
extern ASM_PFX(Bypassed)
extern ASM_PFX(Zero)
extern ASM_PFX(ProgressChars)
extern ASM_PFX(NA)
extern ASM_PFX(CFG)
extern ASM_PFX(Null)
extern ASM_PFX(HookInstalled)
extern ASM_PFX(AtProgress)
extern ASM_PFX(VendorKeys)
extern ASM_PFX(SetupMode)
extern ASM_PFX(SecureBoot)

global HackedRouteConfig

SECTION .text
HackedRouteConfig:
  push    r15
  push    r14
  push    r13
  push    r12
  push    rsi
  push    rdi
  push    rbp
  push    rbx
  sub     rsp, 138h
  mov     rsi, r8
  mov     rdi, rdx
  mov     r15, 8000000000000008h
  call    qword [rel ASM_PFX(OriginalRouteConfig)]
  mov     r14, rax
  mov     rbx, rax
  cmp     rax, r15
  jnz     .loc_16A1
  test    rdi, rdi
  jz      .loc_169E
  movzx   eax, word [rdi]
  test    ax, ax
  jz      .loc_169E
  lea     rcx, [rel ASM_PFX(GlobalVariableGuid)]
  mov     r9d, eax
  mov     rdx, rdi

  .loc_162A:
  xor     r8d, r8d
  nop     dword [rax]

  .loc_1630:
  cmp     r8, 25h
  jz      .loc_1A89
  movzx   r10d, byte [r8+rcx]
  lea     r11d, [r9-41h]
  mov     ebx, r9d
  or      bl, 20h
  cmp     r11w, 1Ah
  movzx   r11d, bl
  cmovnb  r11d, r9d
  mov     r9d, r10d
  add     r9d, 0FFFFFFBFh
  mov     ebx, r10d
  or      bl, 20h
  cmp     r9w, 1Ah
  movzx   r9d, bl
  cmovnb  r9d, r10d
  cmp     r11b, r9b
  jnz     short .loc_168F
  movzx   r9d, word [rdx+r8*2+2]
  inc     r8
  test    r9w, r9w
  jnz     short .loc_1630
  cmp     byte [r8+rcx], 0
  jz      .loc_1A89

  .loc_168F:
  movzx   r9d, word [rdx+2]
  add     rdx, 2
  test    r9w, r9w
  jnz     short .loc_162A

  .loc_169E:
  mov     rbx, r15

  .loc_16A1:
  test    rsi, rsi
  jz      short .loc_16D1
  mov     rax, [rsi]
  xor     r12d, r12d

  .loc_16AC:
  xor     ebp, ebp
  mov     rcx, rax
  mov     r15, 0FFFFFFFFFFFFFFFFh
  sub     rcx, rdi
  jb      short .loc_16DE
  test    rdi, rdi
  jz      short .loc_16DE
  test    rax, rax
  jz      short .loc_16DE
  sar     rcx, 1
  xor     ebp, ebp
  mov     r15, rcx
  jmp     short .loc_16DE

  .loc_16D1:
  mov     r15, 0FFFFFFFFFFFFFFFFh
  mov     bpl, 1
  xor     r12d, r12d

  .loc_16DE:
  mov     rcx, [rel ASM_PFX(FileHandle)]
  test    rcx, rcx
  jz      short .loc_1702
  mov     qword [rsp+30h], 0Bh
  lea     r8, [rel ASM_PFX(Original)]
  lea     rdx, [rsp+30h]
  call    qword [rcx+28h]

  .loc_1702:
  mov     rcx, r14
  call    ASM_PFX(PrintRouteStatus)
  mov     rcx, [rel ASM_PFX(FileHandle)]
  test    rcx, rcx
  jz      short .loc_172E
  mov     qword [rsp+30h], 0Ch
  lea     r8, [rel ASM_PFX(Returned)]
  lea     rdx, [rsp+30h]
  call    qword [rcx+28h]

  .loc_172E:
  mov     rcx, rbx
  call    ASM_PFX(PrintRouteStatus)
  mov     rcx, [rel ASM_PFX(FileHandle)]
  test    rcx, rcx
  jz      short .loc_175A
  mov     qword [rsp+30h], 0Ah
  lea     r8, [rel ASM_PFX(Bypassed)]
  lea     rdx, [rsp+30h]
  call    qword [rcx+28h]

  .loc_175A:
  test    r12, r12
  jz      short .loc_1785
  mov     byte [rsp+30h], 31h
  mov     rcx, [rel ASM_PFX(FileHandle)]
  test    rcx, rcx
  jz      short .loc_17B0
  mov     qword [rsp+28h], 1
  lea     rdx, [rsp+28h]
  lea     r8, [rsp+30h]
  jmp     short .loc_17A6

  .loc_1785:
  mov     rcx, [rel ASM_PFX(FileHandle)]
  test    rcx, rcx
  jz      short .loc_17CD
  mov     qword [rsp+28h], 1
  lea     r8, [rel ASM_PFX(Zero)]
  lea     rdx, [rsp+28h]

  .loc_17A6:
  call    qword [rcx+28h]
  mov     rcx, [rel ASM_PFX(FileHandle)]

  .loc_17B0:
  test    rcx, rcx
  jz      short .loc_17CD
  mov     qword [rsp+30h], 10h
  lea     r8, [rel ASM_PFX(ProgressChars)]
  lea     rdx, [rsp+30h]
  call    qword [rcx+28h]

  .loc_17CD:
  cmp     r15, 0FFFFFFFFFFFFFFFFh
  jz      .loc_186D
  test    r15, r15
  jz      .loc_188B
  xor     r12d, r12d
  mov     rcx, 0CCCCCCCCCCCCCCCDh
  nop     dword [rax]

  .loc_17F0:
  mov     r8, r12
  mov     rax, r15
  mul     rcx
  shr     rdx, 3
  lea     eax, [rdx+rdx]
  lea     eax, [rax+rax*4]
  mov     r9d, r15d
  sub     r9d, eax
  or      r9b, 30h
  mov     byte [rsp+r12+30h], r9b
  inc     r12
  cmp     r8, 16h
  ja      short .loc_1824
  cmp     r15, 9
  mov     r15, rdx
  ja      short .loc_17F0

  .loc_1824:
  mov     rcx, [rel ASM_PFX(FileHandle)]
  lea     r15, [rsp+2Fh]
  lea     r14, [rsp+28h]
  jmp     short .loc_185F

  .loc_1840:
  lea     r8, [r15+r12]
  mov     qword [rsp+28h], 1
  mov     rdx, r14
  call    qword [rcx+28h]
  mov     rcx, [rel ASM_PFX(FileHandle)]
  dec     r12
  jz      short .loc_18AF

  .loc_185F:
  test    rcx, rcx
  jnz     short .loc_1840
  xor     ecx, ecx
  dec     r12
  jnz     short .loc_185F
  jmp     short .loc_18AF

  .loc_186D:
  mov     rcx, [rel ASM_PFX(FileHandle)]
  test    rcx, rcx
  jz      short .loc_18D3
  mov     qword [rsp+30h], 2
  lea     r8, [rel ASM_PFX(NA)]
  jmp     short .loc_18A7

  .loc_188B:
  mov     rcx, [rel ASM_PFX(FileHandle)]
  test    rcx, rcx
  jz      short .loc_18AF
  mov     qword [rsp+30h], 1
  lea     r8, [rel ASM_PFX(Zero)]

  .loc_18A7:
  lea     rdx, [rsp+30h]
  call    qword [rcx+28h]

  .loc_18AF:
  mov     rcx, [rel ASM_PFX(FileHandle)]
  test    rcx, rcx
  jz      short .loc_18D3
  mov     qword [rsp+30h], 5
  lea     r8, [rel ASM_PFX(CFG)]
  lea     rdx, [rsp+30h]
  call    qword [rcx+28h]

  .loc_18D3:
  test    rdi, rdi
  jz      .loc_1975
  movzx   ecx, word [rdi]
  test    cx, cx
  jz      .loc_1999
  xor     r12d, r12d
  mov     r13d, 3Fh
  lea     r14, [rsp+28h]
  lea     r15, [rsp+30h]
  xor     eax, eax
  jmp     short .loc_1910

  .loc_1900:
  xor     eax, eax

  .loc_1902:
  movzx   ecx, word [rdi+r12*2+2]
  inc     r12
  test    cx, cx
  jz      short .loc_1953

  .loc_1910:
  cmp     r12, 400h
  jz      short .loc_1953
  lea     edx, [rcx-20h]
  cmp     dx, 5Fh
  cmovnb  ecx, r13d
  mov     byte [rsp+rax+30h], cl
  inc     rax
  cmp     rax, 100h
  jnz     short .loc_1902
  mov     rcx, [rel ASM_PFX(FileHandle)]
  test    rcx, rcx
  jz      short .loc_1900
  mov     qword [rsp+28h], 100h
  mov     rdx, r14
  mov     r8, r15
  call    qword [rcx+28h]
  jmp     short .loc_1900

  .loc_1953:
  test    rax, rax
  jz      short .loc_1999
  mov     rcx, [rel ASM_PFX(FileHandle)]
  test    rcx, rcx
  jz      short .loc_19BD
  mov     qword [rsp+28h], rax
  lea     rdx, [rsp+28h]
  lea     r8, [rsp+30h]
  jmp     short .loc_1996

  .loc_1975:
  mov     rcx, [rel ASM_PFX(FileHandle)]
  test    rcx, rcx
  jz      short .loc_19BD
  mov     qword [rsp+28h], 6
  lea     r8, [rel ASM_PFX(Null)]
  lea     rdx, [rsp+28h]

  .loc_1996:
  call    qword [rcx+28h]

  .loc_1999:
  mov     rcx, [rel ASM_PFX(FileHandle)]
  test    rcx, rcx
  jz      short .loc_19BD
  mov     qword [rsp+30h], 2
  lea     r8, [rel ASM_PFX(HookInstalled)+29h]
  lea     rdx, [rsp+30h]
  call    qword [rcx+28h]

  .loc_19BD:
  test    bpl, bpl
  jnz     .loc_1CA5
  mov     r14, [rsi]
  test    r14, r14
  jz      .loc_1CA5
  mov     rcx, [rel ASM_PFX(FileHandle)]
  test    rcx, rcx
  jz      short .loc_1A02
  mov     qword [rsp+30h], 0Ch
  lea     r8, [rel ASM_PFX(AtProgress)]
  lea     rdx, [rsp+30h]
  call    qword [rcx+28h]
  mov     r14, [rsi]
  test    r14, r14
  jz      .loc_1C5D

  .loc_1A02:
  movzx   ecx, word [r14]
  test    cx, cx
  jz      .loc_1C81
  mov     r15d, 1
  xor     eax, eax
  mov     ebp, 3Fh
  lea     rsi, [rsp+28h]
  lea     rdi, [rsp+30h]
  jmp     short .loc_1A43

  .loc_1A30:
  xor     eax, eax

  .loc_1A32:
  movzx   ecx, word [r14+r15*2]
  inc     r15
  test    cx, cx
  jz      .loc_1C3B

  .loc_1A43:
  cmp     r15, 0A1h
  jz      .loc_1C3B
  lea     edx, [rcx-20h]
  cmp     dx, 5Fh
  cmovnb  ecx, ebp
  mov     byte [rsp+rax+30h], cl
  inc     rax
  cmp     rax, 100h
  jnz     short .loc_1A32
  mov     rcx, [rel ASM_PFX(FileHandle)]
  test    rcx, rcx
  jz      short .loc_1A30
  mov     qword [rsp+28h], 100h
  mov     rdx, rsi
  mov     r8, rdi
  call    qword [rcx+28h]
  jmp     short .loc_1A30

  .loc_1A89:
  mov     r8d, eax
  mov     rcx, rdi

  .loc_1A8F:
  xor     edx, edx
  db      66h, 66h, 66h, 66h, 66h, 66h, 2Eh
  nop     word [rax+rax+00000000h]

  .loc_1AA0:
  cmp     rdx, 2Eh
  jz      .loc_1C13
  lea     r9, [rel ASM_PFX(VendorKeys)]
  movzx   r10d, byte [rdx+r9]
  lea     r11d, [r8-41h]
  mov     ebx, r8d
  or      bl, 20h
  cmp     r11w, 1Ah
  movzx   r11d, bl
  cmovnb  r11d, r8d
  mov     r8d, r10d
  add     r8d, 0FFFFFFBFh
  mov     ebx, r10d
  or      bl, 20h
  cmp     r8w, 1Ah
  movzx   r8d, bl
  cmovnb  r8d, r10d
  cmp     r11b, r8b
  jnz     short .loc_1B06
  movzx   r8d, word [rcx+rdx*2+2]
  inc     rdx
  test    r8w, r8w
  jnz     short .loc_1AA0
  cmp     byte [rdx+r9], 0
  jz      .loc_1C13

  .loc_1B06:
  movzx   r8d, word [rcx+2]
  add     rcx, 2
  test    r8w, r8w
  jnz     .loc_1A8F
  lea     rcx, [rel ASM_PFX(SetupMode)]
  mov     r9d, eax
  mov     rdx, rdi

  .loc_1B26:
  xor     r8d, r8d
  nop     dword [rax+00000000h]

  .loc_1B30:
  cmp     r8, 2Ah
  jz      .loc_1C13
  movzx   r10d, byte [r8+rcx]
  lea     r11d, [r9-41h]
  mov     ebx, r9d
  or      bl, 20h
  cmp     r11w, 1Ah
  movzx   r11d, bl
  cmovnb  r11d, r9d
  mov     r9d, r10d
  add     r9d, 0FFFFFFBFh
  mov     ebx, r10d
  or      bl, 20h
  cmp     r9w, 1Ah
  movzx   r9d, bl
  cmovnb  r9d, r10d
  cmp     r11b, r9b
  jnz     short .loc_1B8F
  movzx   r9d, word [rdx+r8*2+2]
  inc     r8
  test    r9w, r9w
  jnz     short .loc_1B30
  cmp     byte [r8+rcx], 0
  jz      .loc_1C13

  .loc_1B8F:
  movzx   r9d, word [rdx+2]
  add     rdx, 2
  test    r9w, r9w
  jnz     short .loc_1B26
  lea     rcx, [rel ASM_PFX(SecureBoot)]
  mov     rdx, rdi

  .loc_1BA8:
  xor     r8d, r8d

  .loc_1BAB:
  cmp     r8, 2Eh
  jz      short .loc_1C13
  movzx   r9d, byte [r8+rcx]
  lea     r10d, [rax-41h]
  mov     r11d, eax
  or      r11b, 20h
  cmp     r10w, 1Ah
  movzx   r10d, r11b
  cmovnb  r10d, eax
  mov     eax, r9d
  add     eax, 0FFFFFFBFh
  mov     r11d, r9d
  or      r11b, 20h
  cmp     ax, 1Ah
  movzx   eax, r11b
  cmovnb  eax, r9d
  cmp     r10b, al
  jnz     short .loc_1C01
  movzx   eax, word [rdx+r8*2+2]
  inc     r8
  test    ax, ax
  jnz     short .loc_1BAB
  cmp     byte [r8+rcx], 0
  jz      short .loc_1C13

  .loc_1C01:
  movzx   eax, word [rdx+2]
  add     rdx, 2
  test    ax, ax
  jnz     short .loc_1BA8
  jmp     .loc_169E

  .loc_1C13:
  test    rsi, rsi
  jz      .loc_1CCB
  lea     rax, [rdi-2]

  .loc_1C20:
  cmp     word [rax+2], 0
  lea     rax, [rax+2]
  jnz     short .loc_1C20
  mov     [rsi], rax
  mov     r12d, 1
  xor     ebx, ebx
  jmp     .loc_16AC

  .loc_1C3B:
  test    rax, rax
  jz      short .loc_1C81
  mov     rcx, [rel ASM_PFX(FileHandle)]
  test    rcx, rcx
  jz      short .loc_1CB4
  mov     qword [rsp+28h], rax
  lea     rdx, [rsp+28h]
  lea     r8, [rsp+30h]
  jmp     short .loc_1C7E

  .loc_1C5D:
  mov     rcx, [rel ASM_PFX(FileHandle)]
  test    rcx, rcx
  jz      short .loc_1CB4
  mov     qword [rsp+28h], 6
  lea     r8, [rel ASM_PFX(Null)]
  lea     rdx, [rsp+28h]

  .loc_1C7E:
  call    qword [rcx+28h]

  .loc_1C81:
  mov     rcx, [rel ASM_PFX(FileHandle)]
  test    rcx, rcx
  jz      short .loc_1CB4
  mov     qword [rsp+30h], 2
  lea     r8, [rel ASM_PFX(HookInstalled)+29h]
  lea     rdx, [rsp+30h]
  call    qword [rcx+28h]

  .loc_1CA5:
  mov     rcx, [rel ASM_PFX(FileHandle)]
  test    rcx, rcx
  jz      short .loc_1CB4
  call    qword [rcx+50h]

  .loc_1CB4:
  mov     rax, rbx
  add     rsp, 138h
  pop     rbx
  pop     rbp
  pop     rdi
  pop     rsi
  pop     r12
  pop     r13
  pop     r14
  pop     r15
  ret

  .loc_1CCB:
  mov     r15, 0FFFFFFFFFFFFFFFFh
  mov     r12d, 1
  mov     bpl, 1
  xor     ebx, ebx
  jmp     .loc_16DE

