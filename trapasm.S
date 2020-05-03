#include "mmu.h"
#include "traps.h"
#include "syscall.h"

  # vectors.S sends all traps here.
.globl alltraps
alltraps:
  # Build trap frame.
  pushl %ds
  pushl %es
  pushl %fs
  pushl %gs
  pushal
  
  # Set up data segments.
  movw $(SEG_KDATA<<3), %ax
  movw %ax, %ds
  movw %ax, %es

  # Call trap(tf), where tf=%esp
  pushl %esp
  call trap
  addl $4, %esp

  # Return falls through to trapret...
.globl trapret
trapret:
  push %esp
  call execPendings
  addl $4, %esp
  popal
  popl %gs
  popl %fs
  popl %es
  popl %ds
  addl $0x8, %esp  # trapno and errcode
  iret

 # Call to sigret - code to copy
.globl call_sigret
.globl call_sigret_end
call_sigret:
  movl SYS_sigret, %eax
  int $T_SYSCALL
  ret
call_sigret_end: